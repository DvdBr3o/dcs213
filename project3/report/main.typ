#import "style.typ": *

#set text(lang: "cn")

#show: dvdbr3otypst.with(
  title: [DCS213 Project3 \ 实验报告],
  subtitle: [],
  author: [代骏泽],
  xno: [24363012],
  // bib: bibliography("reference.bib"),
)

= 程序功能简要说明

本程序是一个基于 **Huffman编码** 的图像压缩工具，能够对BMP、JPG、PNG、PPM等多种格式的图像进行无损压缩，并可将压缩文件解压恢复为原始图像。程序实现了完整的Huffman编码流程，包括像素频率统计、Huffman树构建、编码表生成、数据压缩存储以及解压还原。

== 核心功能模块

- **图像读取与预处理**：使用OpenCV库读取图像，支持灰度图与彩色图（最多4个通道）。对每个通道分别统计像素值（0-255）的出现频率。
- **Huffman树构建**：根据各像素值的频率，构建最优前缀码树。使用优先队列（最小堆）合并节点，确保频率低的节点在树中路径较长。
- **编码表生成**：遍历Huffman树，为每个像素值生成对应的二进制编码，编码长度可变，频率高的像素使用短编码。
- **压缩存储**：将图像的每个像素替换为其Huffman编码，并将编码后的比特流按字节打包，最后与编码表、图像尺寸等信息序列化保存为二进制文件。
- **解压功能**：读取压缩文件，反序列化得到编码表和图像尺寸，根据编码表逐比特解码恢复原始像素值，重建图像并保存。
- **性能分析**：计算压缩比（压缩后大小/原始大小）、编码与解码时间，并在控制台输出。

= 程序运行截图与演示

== 命令行界面

#figure(caption: [命令行界面])[#image("cli.png")]

程序通过命令行参数交互，支持以下命令：

- `encode <图像路径> -o <输出路径>`：压缩图像
- `decode <压缩文件路径> -o <输出图像路径>`：解压图像
- `test <图像路径>`：测试压缩与解压的完整性
- `help`：显示帮助信息

示例运行（在项目根目录下）：

```bash
# 压缩 house.tif 图像
build/windows/x64/debug/dcs213.project3.exe encode public/standard_test_images/house.tif -o house.huf

# 解压 house.huf 文件
build/windows/x64/debug/dcs213.project3.exe decode house.huf -o house_reconstructed.png
```

== 运行效果

程序运行时会在控制台输出压缩比与时间信息，例如：

```
encode done!
encode time usage: 123.456ms
compress ratio: 45.23%
```

同时，程序会通过OpenCV窗口显示原始图像与解压后的图像，供用户直观对比。

#figure(caption: [测试结果显示])[#image("show.png")]

= 部分关键代码及其说明

== Huffman树节点与编码表（Codec.hpp）

```cpp
struct HuffmanNode {
  Freq f;
  const HuffmanNode* l = nullptr;
  const HuffmanNode* r = nullptr;

  // 递归生成编码表
  void gen_map(HuffmanChannel& map, std::uint32_t len = 0, std::uint32_t code = 0) const {
    if (l) l->gen_map(map, len + 1, ((code << 1) | std::uint32_t(0)));
    if (r) r->gen_map(map, len + 1, ((code << 1) | std::uint32_t(1)));
    if (f.v >= 0) map.map[f.v] = { len, (code & ((1 << len) - 1)) };
  }
};
```

**说明**：`HuffmanNode` 表示树中的一个节点，包含频率 `f` 和左右子节点指针。`gen_map` 方法采用深度优先遍历，每当到达叶子节点（`f.v >= 0`）时，将当前累积的编码长度 `len` 和编码值 `code` 存入编码表。左分支追加比特0，右分支追加比特1。

== 构建Huffman树（Codec.hpp）

```cpp
inline static auto from(const cv::Mat& mat) -> HuffmanChannel {
  // 统计频率
  std::array<Freq, 256> freq = {0};
  for (int i = 0; i < freq.size(); ++i) freq[i].v = i;
  for (int i = 0; i < mat.size[0]; ++i)
    for (int j = 0; j < mat.size[1]; ++j) 
      ++freq[mat.at<uchar>(i, j)].freq;

  // 初始化叶子节点
  std::vector<HuffmanNode> ns;
  for (const auto& f : freq) ns.emplace_back(f, nullptr, nullptr);

  // 优先队列（最小堆）
  std::priority_queue<QueueNode, std::vector<QueueNode>, std::greater<>> q;
  for (const auto& n : ns) q.emplace(&n);

  // 合并节点
  while (q.size() > 1) {
    auto l = q.top(); q.pop();
    auto r = q.top(); q.pop();
    ns.emplace_back(l->f + r->f, l.p, r.p);
    q.emplace(&ns.back());
  }
  // 生成编码表
  HuffmanChannel chan;
  q.top()->gen_map(chan);
  return chan;
}
```

**说明**：首先统计图像中每个像素值（0‑255）的出现频率。将所有频率构建为叶子节点放入优先队列（按频率升序）。循环地从队列中取出两个频率最小的节点，合并为一个新节点（其频率为两者之和），并将新节点放回队列。重复直到队列只剩一个节点，即Huffman树的根节点。最后调用 `gen_map` 从根节点生成编码表。

== 编码与打包（Codec.hpp）

```cpp
std::vector<bool> bitstream;
bitstream.reserve(mat.size[0] * mat.size[1]);
for (int i = 0; i < mat.size[0]; ++i)
  for (int j = 0; j < mat.size[1]; ++j) {
    const auto huf = chan[mat.at<uchar>(i, j)];
    for (int i = 0; i < huf.len; ++i)
      bitstream.emplace_back(((1 << (huf.len - i - 1)) & huf.huff));
  }

// 将比特流打包为字节
for (int b = 0; b <= bitstream.size() / 8; ++b) {
  uchar curby = 0;
  for (int i = 0; i < 8; ++i)
    if (8 * b + i < bitstream.size())
      curby |= (bitstream[8 * b + i] << (8 - i - 1));
  chan.data.emplace_back(curby);
}
```

**说明**：遍历图像的每个像素，查表得到其Huffman编码，将编码的每个比特依次存入 `bitstream`。之后，每8个比特打包为一个字节，高位在前（`<< (8 - i - 1)`）。若比特流长度不是8的倍数，最后一个字节的高位补零。

== 解码流（Codec.hpp）

```cpp
struct DecodeStream {
  std::uint32_t maxb = 0;
  std::map<HuffmanVal, uchar> map;  // 反向查找表：编码 -> 像素值
  std::span<const uchar> data;
  std::size_t b = 0;    // 当前字节索引
  std::size_t inb = 0;  // 当前字节内的比特位置

  auto bump() -> uchar {
    HuffmanVal huff = {};
    for (std::uint32_t l = 1; l <= maxb; ++l) {
      if (b >= data.size()) return 0;
      huff = huff.cat(data[b] & (1 << (8 - inb - 1)));  // 取下一个比特
      ++inb;
      b += inb / 8;
      inb %= 8;
      auto it = map.find(huff);
      if (it != map.end()) return it->second;  // 找到对应像素值
    }
    return 0;
  }
};
```

**说明**：`DecodeStream` 维护一个从编码到像素值的反向映射表。`bump()` 方法逐比特读取压缩数据，并累加到 `huff` 中，每读一个比特就查询一次映射表，一旦匹配到合法编码立即返回对应的像素值。这种实现保证了前缀码的无歧义解码。

== 命令行解析（CommandLine.hpp）

```cpp
const auto encode = (command("encode").set(_mode, Mode::Encode),
                     value("img_path").set(_img_path),
                     output)
                  % "Encode images";
const auto decode = (command("decode").set(_mode, Mode::Decode),
                     value("img_path").set(_img_path),
                     output)
                  % "Decode images";
const auto cli = (help | test | encode | decode);

if (!parse(argc, argv, cli))
  std::cout << make_man_page(cli);

switch (_mode) {
  case Mode::Encode: encode_halffman(_img_path, _output_path); break;
  case Mode::Decode: decode_halffman(_img_path, _output_path); break;
  // ...
}
```

**说明**：使用 `clipp` 库定义命令行语法。`command("encode")` 匹配子命令，`value("img_path")` 捕获图像路径，`output` 捕获 `-o` 参数。解析成功后根据模式调用相应的编码或解码函数。

= 程序运行方式简要说明

== 环境依赖

- **编译器**：支持C++20的编译器（如MSVC、GCC、Clang）
- **构建系统**：xmake（配置文件见 `xmake.lua`）
- **第三方库**：
  + OpenCV（图像读取/显示）
  + ylt/struct_pack（序列化）
  + clipp（命令行解析）
  + stb_image（备用图像加载）

== 构建步骤

在项目根目录执行：

```bash
xmake config --mode=debug  # 或 --mode=release
xmake build -v
```

编译后的可执行文件位于 `build/windows/x64/debug/dcs213.project3.exe`。

== 使用示例

1. **压缩图像**：
   ```bash
   dcs213.project3.exe encode public/standard_test_images/lena_gray_512.tif -o lena.huf
   ```

2. **解压图像**：
   ```bash
   dcs213.project3.exe decode lena.huf -o lena_reconstructed.png
   ```

3. **测试完整性**：
   ```bash
   dcs213.project3.exe test public/standard_test_images/peppers_color.tif
   ```
   该命令会压缩再解压，并显示原始图像与重建图像。

== 性能指标

程序会在控制台输出：
- **压缩时间**：编码过程所耗时间。
- **压缩比**：压缩后文件大小与原始图像大小的百分比。

== 注意事项

- 本程序为**无损压缩**，解压后的图像与原始图像完全相同（像素级一致）。
- 支持灰度图（单通道）与彩色图（3通道RGB或4通道RGBA）。
- 压缩文件后缀建议使用 `.huf`，但并非强制。

= 总结

本次实验实现了一个完整的基于Huffman编码的图像压缩程序，涵盖了从频率统计、树构建、编码生成到数据打包、序列化存储以及解压恢复的全过程。通过手动实现Huffman算法，加深了对**前缀码**、**最优编码**以及**无损压缩**原理的理解。程序结构清晰，模块划分合理，并提供了友好的命令行接口与实时的性能反馈，符合实验要求。
