#import "style.typ": *

#set text(lang: "cn")

#show: dvdbr3otypst.with(
  title: [DCS213 Project2 \ 实验报告],
  subtitle: [],
  author: [代骏泽],
  xno: [24363012],
  bib: bibliography("reference.bib"),
)

= 功能介绍

整体形式是一个 CLI，指定 `--show` 时会展示原始图片和处理后图片.

CLI 概览如下:

```bash
❯ xmake r dcs213.project2 --help
SYNOPSIS
        grayscale <image> --output <output> [--show|--hide]
        rescale <image> --output <output> --size <size> [--show|--hide]
        encode <image> --output <output> [--show|--hide]
        decode <image> --output <output> [--show|--hide]
        help [--show|--hide]
        show <image>
```

== 读取显示 Show

使用指令 `show`，后面紧跟所需展示图片的地址，即可在图形化界面内展示该图片.

```bash
dcs213.project2 show color-block.png
```

#figure(caption: [读取显示图片])[
	#rounded(image("show.png", width: 60%))
]

== 灰度化 Grayscaling

使用指令 `grayscale`，指定需要灰度化的图片，再用 `-o`/`--output` 指定输出路径，即可实现把一个图片灰度化. 添加选项 `--show` 可以在图形化界面显示灰度化结果.

```bash
dcs213.project2 grayscale color-block.ppm -o color-block-grayscaled.png --show
```

#figure(caption: [图片灰度化])[
	#rounded(image("grayscale.png", width: 60%))
]

== 缩放 Rescaling

使用指令 `rescale`，指定需要缩放的图片，再用 `-o`/`--output` 指定输出路径，最后用 `-s` 指定缩放到的大小 (格式为 `width x height`, 无空格)，即可实现图片的缩放. 添加选项 `--show` 可以在图形化界面显示缩放结果.

#info[
  由于实验不要求，所以该功能只支持整数倍缩放.
]

=== 放大 Zoom in

```bash
dcs213.project2 rescale lena-128-gray.png -o lena-128-gray-rescaled.png -s 256x256 --show
```

#figure(caption: [放大图片])[
	#rounded(image("zoomin.png", width: 60%))
]

=== 缩小 Zoom out

```bash
dcs213.project2 rescale lena-128-gray.png -o lena-128-gray-rescaled.png -s 256x256 --show
```

#figure(caption: [缩小图片])[
	#rounded(image("zoomout.png", width: 50%))
]

== 编解码 Codec

使用指令 `encode`/`decode`，指定需要编解码的图片，再用 `-o`/`--output` 指定输出路径，即可实现图片的压缩/解压缩的编解码. 编码格式后缀为 `.tri`，编码格式为实验要求的三元组存储. 具体编码原理见 @codec_how.

#info[
  虽然实验报告说三元组存储是一种压缩实现，但是实际实现下来三元组存储的大小比 `png` 等格式大很多...
]

=== 编码 Encode

```bash
dcs213.project2 encode color-block.png -o color-block.tri
```

=== 解码 Decode

```bash
dcs213.project2 decode color-block.tri -o color-block-decoded.png
```

= 原理解析

== 命令行

该项目用 `clipp` (见 @clipp) 实现命令行组织. 大概结构如下：

```cpp
namespace dcs213::project2 {
	class CommandLine {
	public:
		enum class Mode {
			Help,
			Grayscale,
			Rescale,
			Encode,
			Decode,
			Show,
		};

	public:
		CommandLine(int argc, char** argv) {
			using namespace clipp;

			auto image	   = value("image").set(_image_path);
			auto output	   = (required("--output", "-o"), value("output").set(_output_path));

			auto grayscale = // grayscale subcommand

			auto rescale = // rescale subcommand

			auto encode = // encode subcommand

			auto decode = // decode subcommand

			auto help	= command("help", "--help", "-h").set(_mode, Mode::Help);

			auto cli =
				((grayscale | rescale | encode | decode | help,
				  option("--show").set(_show, true) | option("--hide").set(_show, false))
				 | (command("show").set(_mode, Mode::Show), image));

			if (!parse(argc, argv, cli)) // fail to parse
				std::cout << make_man_page(cli);

			switch (_mode) {
				case Mode::Grayscale: 
          // do grayscale
          break;
				case Mode::Encode: 
          // do encode
          break;
				case Mode::Decode: 
          // do decode
          break;
				case Mode::Rescale:
					// do rescale
					break;
				case Mode::Help: 
          std::cout << make_man_page(cli) << '\n'; break;
				case Mode::Show:
					// do show
					break;
			}
		}

	private:
		Mode		    _mode;
		std::string _image_path;
		std::string _output_path;
		std::string _rescale_size;
		bool		    _show;
	};
}  // namespace dcs213::project2
```

== 读取显示 Show

with OpenCV 这其实很简单. 读取图片用 `cv::imread()`，并用 `cv::imshow()` 来显示图片即可.

子命令构造:

```cpp
auto show = (command("show").set(_mode, Mode::Show), image);
```

实际逻辑:

```cpp
// ...
case Mode::Show:
  cv::imshow(_image_path, cv::imread(_image_path));
  cv::waitKey(0);
  break;
```

== 灰度化 Grayscaling

根据实验手册给出的 @grayscale_techs 参考中，可以知道彩色图像转灰度图像的一种公式：

$
  g=.299r + .587g + .114b
$

从而可以给出灰度化的实现：
```cpp
	inline static void
		grayscale(const std::string& image_path, const std::string& output_path, bool show = true) {
		const auto img = cv::imread(image_path);

		cv::Mat	   res;
		img.copyTo(res);

		res.forEach<cv::Vec3b>([](cv::Vec3b& pixel, const int* pos) {
			const unsigned char gray = .299 * pixel[0] + .587 * pixel[1] + .114 * pixel[2];
			pixel[0]				 = gray;
			pixel[1]				 = gray;
			pixel[2]				 = gray;
		});

		cv::imwrite(output_path, res);

		if (show) {
			cv::imshow(image_path, img);
			cv::imshow(output_path, res);
			cv::waitKey(0);
		}
	}
```

#info[
  使用 `.forEach()` 从而实现并行 `for-loop`. 当然用 STL 的 `std::execution::par` 也是可以的，但是一般性能没有 OpenCV 自己的好.
]

== 缩放 Rescaling

不考虑非整数倍缩放.

=== 放大 Zoom in

记 `scale` 为缩放倍率，结果数组的每个 `([i * scale .. (i + 1) * scale], [j * scale .. ((j + 1) * scale)])` 矩形区域都赋值为原图像数组的 `(i, j)`，最后在进行一次双线性插值即可.

根据实验报告给出的链接指导 @bilinear_interpolation，双线性插值原理如下：

$
	f_(r)(i, j) = \
		f(floor(i times #scale), floor(j times #scale)) (1-{i times #scale})(1-{j times #scale}) + \
		f(floor(i times #scale), ceil(j times #scale)) (1-{i times #scale}){j times #scale} + \
		f(ceil(i times #scale), floor(j times #scale)) {i times #scale}(1-{j times #scale}) + \
		f(ceil(i times #scale), ceil(j times #scale)) {i times #scale}{j times #scale}
$

实际上就是单维度线性插值扩展到两个维度.

实现如下:

```cpp
inline static void rescale(
		const std::string& image_path,
		const std::string& output_path,
		const RescaleSize& size,
		bool			   show = false
	) {
		const auto img = cv::imread(image_path);

		// ...

		if (img.size[0] > size.height) {  // zoom out
			// ...
		} else {  // zoom in
			const auto scale = size.height / img.size[0];
			const auto sep	 = 1. / static_cast<double>(scale);
			for (int i = 0; i < img.size[0] - 1; ++i)
				for (int j = 0; j < img.size[1] - 1; ++j) {
					for (int x = 0; x < scale; ++x)
						for (int y = 0; y < scale; ++y)
							res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
								img.at<cv::Vec3b>(i, j) * (1 - sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i, j + 1) * (1 - sep * x) * (sep * y) +  //
								img.at<cv::Vec3b>(i + 1, j) * (sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i + 1, j + 1) * (sep * x) * (sep * y)	   //
								;
				}
			{
				const int i = img.size[0] - 1;
				for (int j = 0; j < img.size[1] - 1; ++j) {
					for (int x = 0; x < scale; ++x)
						for (int y = 0; y < scale; ++y)
							res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
								img.at<cv::Vec3b>(i - 1, j) * (1 - sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i - 1, j + 1) * (1 - sep * x) * (sep * y) +  //
								img.at<cv::Vec3b>(i, j) * (sep * x) * (1 - sep * y) +		   //
								img.at<cv::Vec3b>(i, j + 1) * (sep * x) * (sep * y)			   //
								;
				}
			}
			{
				const int j = img.size[1] - 1;
				for (int i = 0; i < img.size[1] - 1; ++i) {
					for (int x = 0; x < scale; ++x)
						for (int y = 0; y < scale; ++y)
							res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
								img.at<cv::Vec3b>(i, j - 1) * (1 - sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i, j) * (1 - sep * x) * (sep * y) +		   //
								img.at<cv::Vec3b>(i + 1, j - 1) * (sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i + 1, j) * (sep * x) * (sep * y)			   //
								;
				}
			}
			{
				const int i = img.size[0] - 1;
				const int j = img.size[1] - 1;
				for (int x = 0; x < scale; ++x)
					for (int y = 0; y < scale; ++y)
						res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
							img.at<cv::Vec3b>(i - 1, j - 1) * (1 - sep * x) * (1 - sep * y) +  //
							img.at<cv::Vec3b>(i - 1, j) * (1 - sep * x) * (sep * y) +		   //
							img.at<cv::Vec3b>(i, j - 1) * (sep * x) * (1 - sep * y) +		   //
							img.at<cv::Vec3b>(i, j) * (sep * x) * (sep * y)					   //
							;
			}
		}

		cv::imwrite(output_path, res);

		if (show) {
			cv::imshow(image_path, img);
			cv::imshow(output_path, res);
			cv::waitKey(0);
		}
}
```

多个 `for` 循环是为了手动消除边界情况的分支判断，从而减少分支预测失败，提高性能. 主要逻辑是处理边界情况，避免对原数组访问越界. 解决办法就是把边界像素组的参考点从 `((i..i+1), (j..j+1))` 变成 `((i-1..i), (j-1..j))`.

=== 缩小 Zoom out

记 `scale` 为缩小倍率，所以只需要求数组每个以 `scale * (width, height)` 的矩形区域内的平均值作为对应结果图像的值即可.

```cpp
inline static void rescale(
		const std::string& image_path,
		const std::string& output_path,
		const RescaleSize& size,
		bool			   show = false
	) {
		const auto img = cv::imread(image_path);
		
    // ...

		cv::Mat res = cv::Mat::zeros(size.height, size.width, CV_8UC3);

		// TODO: interpolation
		if (img.size[0] > size.height) {  // zoom out
			const auto scale = img.size[0] / size.height;
			for (int i = 0; i < size.height; ++i)
				for (int j = 0; j < size.width; ++j) {
					cv::Vec3f pixel = 0;
					img(cv::Rect { static_cast<int>(j * scale),
								   static_cast<int>(i * scale),
								   static_cast<int>(scale),
								   static_cast<int>(scale) })
						.forEach<cv::Vec3b>([&](const cv::Vec3b& p, const int* pos) { pixel += p; }
						);
					pixel *= (1. / (scale * scale));
					res.at<cv::Vec3b>(i, j) = pixel;
				}
		} else {  // zoom in
			// ...
		}

		cv::imwrite(output_path, res);

		if (show) {
			cv::imshow(image_path, img);
			cv::imshow(output_path, res);
			cv::waitKey(0);
		}
	}
```

== 编解码 Codec <codec_how>

根据实验要求，编码格式采用三元组. 编码开头存储总大小以及各通道三元组数量，末尾存储一个浮动大小的字节数组. 另外，本项目只考虑 RGB 三通道图像. 

```cpp
struct alignas(std::size_t) RawTriplet {
		std::size_t total_size;
		std::size_t w;
		std::size_t h;
		// red channel triplets
		std::size_t rc_size;
		// green channel triplets
		std::size_t gc_size;
		// blue channel triplets
		std::size_t bc_size;
		// dynamic sizs data
		std::byte data[];

		using Ptr = std::unique_ptr<
        RawTriplet, 
        decltype([](RawTriplet* p) {
					  delete[] reinterpret_cast<char*>(p);
				})
    >;
    // ...
};
```

=== 编码 Encode

先读取图像，再对每个通道值非 `0` 的像素个数计数，得到各通道三元组数组大小，得到各通道三元组在 `RawTriplet::data` 里的偏移量. 然后再对每个通道的非 `0` 像素进行三元组编码并存储到 `data` 中. 

```cpp
inline static auto from(const cv::Mat& mat) -> Ptr {
			std::vector<cv::Mat> channels;
			cv::split(mat, channels);

			std::size_t sizes[3] = { 0 };
			std::size_t offsets[3];

			for (int c = 0; c < 3; ++c)
				for (int i = 0; i < channels[c].size[0]; ++i)
					for (int j = 0; j < channels[c].size[1]; ++j)
						if (channels[c].at<uchar>(i, j))
							++sizes[c];

			Ptr ptr { reinterpret_cast<RawTriplet*>(
				new char
					[offsetof(RawTriplet, data)
					 + (sizes[0] + sizes[1] + sizes[2]) * sizeof(Triplet<uchar>)]
			) };

			ptr->h			= mat.rows;
			ptr->w			= mat.cols;

			ptr->rc_size	= sizes[0];
			ptr->gc_size	= sizes[1];
			ptr->bc_size	= sizes[2];

			ptr->total_size = sizeof(RawTriplet) - sizeof(std::byte*)
							+ (ptr->rc_size + ptr->gc_size + ptr->bc_size) * sizeof(Triplet<uchar>);

			offsets[0] = 0;
			offsets[1] = sizes[0];
			offsets[2] = sizes[0] + sizes[1];

			for (int c = 0; c < 3; ++c) {
				int index = 0;
				for (std::size_t i = 0; i < channels[c].size[0]; ++i)
					for (std::size_t j = 0; j < channels[c].size[1]; ++j)
						if (const auto pixel = channels[c].at<uchar>(i, j))
							*(reinterpret_cast<Triplet<uchar>*>(ptr->data) + (offsets[c] + index++)
							) = { .x = i, .y = j, .data = pixel };
			}

			return ptr;
		}
```

得到了三元组存储后，用 `.to_file()` 存储到指定路径：

```cpp
void to_file(const std::filesystem::path& path) const {
    std::ofstream out { path, std::ios::binary };

		if (!out.is_open())
				throw std::runtime_error { std::format("Failed to open path {}!", path.string()) };

		out.write(reinterpret_cast<const char*>(this), total_size);

		out.close();
}
```


=== 解码 Decode

首先从指定路径读取二进制数据，然后将其解释为 `RawTriplet` 类型，通过头部的内存大小信息得到各通道的三元组内存布局，接下来就可以根据三元组还原出像素矩阵，即原图像了.

```cpp
[[nodiscard]] auto to_mat() const -> cv::Mat {
			cv::Mat		mat = cv::Mat::zeros(h, w, CV_8UC3);

			std::size_t sizes[3];
			std::size_t offsets[3];

			sizes[0]   = rc_size;
			sizes[1]   = gc_size;
			sizes[2]   = bc_size;
			offsets[0] = 0;
			offsets[1] = rc_size;
			offsets[2] = rc_size + gc_size;

			for (int c = 0; c < 3; ++c)
				for (int i = 0; i < sizes[c]; ++i) {
					const auto& triplet =
						*(reinterpret_cast<const Triplet<uchar>*>(data) + offsets[c] + i);
					mat.at<cv::Vec3b>(	//
						static_cast<int>(triplet.x),
						static_cast<int>(triplet.y)
					)[c] = triplet.data;
				}

			return mat;
		}
```

= 复现指引

依旧 `xmake`.

```bash
xmake config -m release -y
xmake build dcs213.project2
```

然后可以直接用 `scripts/` 目录下的脚本快速测试实验要求中的测试图集.

```bash
nu scripts/show.nu      # 读取显示
nu scripts/grayscale.nu # 灰度化
nu scripts/zoomin.nu    # 放大
nu scripts/zoomout.nu   # 缩小
nu scripts/encode.nu    # 压缩成三元组
nu scripts/decode.nu    # 从三元组还原
```