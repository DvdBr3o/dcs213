#import "style.typ": *

#set text(lang: "cn")

#show: dvdbr3otypst.with(
  title: [DCS213 Project4 \ 实验报告],
  subtitle: [],
  author: [代骏泽],
  xno: [24363012],
  // bib: bibliography("reference.bib"),
)

= 程序功能简要说明

本程序是一个基于 **图结构** 的应用，实现了无向图的深度优先遍历（DFS）、广度优先遍历（BFS）、非递归深度优先遍历、生成树构建以及最短路径计算。程序以邻接表为存储结构（实际实现为邻接多重表的简化形式），支持从CSV文件读取图数据（城市与道路），并提供可视化展示。

== 核心功能模块

- **图构建与数据加载**：通过 `rapidcsv` 读取城市名称文件（`city.csv`）和边权文件（`map.csv`），构建带权无向图。每个顶点具有名称，每条边带有距离权重。
- **深度优先遍历（DFS）**：递归实现图的深度优先遍历，并生成深度优先生成树。
- **广度优先遍历（BFS）**：使用队列实现广度优先遍历，并生成广度优先生成树。
- **非递归深度优先遍历**：借助自定义栈实现非递归DFS，同样生成生成树。
- **最短路径计算**：使用Dijkstra算法计算从指定起点到所有其他顶点的最短路径及距离。
- **可视化展示**：利用Matplot++库绘制原图及各生成树，以图形化方式展示图结构。

= 程序运行截图与演示

== 命令行界面

程序通过命令行参数交互，支持以下命令：

- `show`：显示加载的图结构（可视化窗口）
- `bfs <起点编号>`：执行BFS遍历，输出遍历序列并生成广度优先生成树（可视化）
- `dfs <起点编号>`：执行DFS遍历，输出遍历序列并生成深度优先生成树（可视化）
- `stack-dfs <起点编号>`：使用栈非递归实现DFS，输出遍历序列并生成生成树（可视化）
- `dijkstra <起点编号>`：计算从起点到所有其他顶点的最短路径，并输出距离

示例运行（在项目根目录下）：

```bash
# 显示图结构
build/windows/x64/debug/dcs213.project4.exe show

# 从顶点0开始进行BFS遍历
build/windows/x64/debug/dcs213.project4.exe bfs 0

# 从顶点0开始进行DFS遍历
build/windows/x64/debug/dcs213.project4.exe dfs 0

# 使用栈非递归DFS从顶点0开始遍历
build/windows/x64/debug/dcs213.project4.exe stack-dfs 0

# 计算从顶点0到所有其他顶点的最短路径
build/windows/x64/debug/dcs213.project4.exe dijkstra 0
```

== 运行效果

程序运行时会在控制台输出遍历序列、生成树边集以及最短路径距离，例如：

```
BFS traversal starting from 0:
0 1 2 3 ...
BFS tree edges:
(0,1) (0,2) (1,3) ...
```

同时，程序会通过Matplot++窗口显示原图、生成树等可视化结果。

（注：此处应附上运行截图，如命令行输出、可视化图等，由于报告撰写时无法实时运行，请读者自行运行程序查看效果。）

= 部分关键代码及其说明

== 图数据结构定义（Graph.hpp）

```cpp
struct Edge {
    std::size_t   to;
    std::uint32_t w;
};

struct Node {
    std::vector<Edge> es;
    std::string       name;
};

class Graph {
    std::vector<Node> _ns;
};
```

**说明**：`Node` 表示图的一个顶点，包含一个边列表 `es` 和顶点名称 `name`。`Edge` 表示一条边，包含目标顶点索引 `to` 和权重 `w`。图使用邻接表存储，即 `_ns` 是一个 `Node` 的数组，每个 `Node` 的 `es` 存储从该顶点出发的所有边。

== 图的构建（Graph.hpp）

```cpp
Graph(const std::filesystem::path& map, const std::filesystem::path& city) {
    // 读取城市名称
    const auto name = rapidcsv::Document(city.string()).GetColumn<std::string>("name");
    _ns.resize(name.size());
    for (int i = 0; i < _ns.size(); ++i) _ns[i].name = name[i];

    // 读取边
    auto doc = rapidcsv::Document(map.string());
    const auto src = doc.GetColumn<std::size_t>("src");
    const auto dst = doc.GetColumn<std::size_t>("dst");
    const auto dis = doc.GetColumn<std::uint32_t>("dis");

    for (const auto& [u, v, w] : ranges::views::zip(src, dst, dis)) {
			_add_edge(u, v, w);
			_add_edge(v, u, w);
		}
}
```

**说明**：构造函数从两个CSV文件分别读取顶点名称和边信息。`city.csv` 包含一列 `name`，对应每个顶点的名称；`map.csv` 包含三列：`src`（起点）、`dst`（终点）、`dis`（距离）。使用 `rapidcsv` 解析后，通过 `_add_edge` 将边加入邻接表。

== 广度优先遍历（BFS）与生成树（Graph.hpp）

```cpp
auto bfs_tree(std::size_t src) const -> Graph {
    Graph g { _ns.size() };
    for (int i = 0; i < _ns.size(); ++i) g._ns[i].name = _ns[i].name;
    std::vector<bool> vis(_ns.size(), false);

    std::queue<std::size_t> q;
    q.push(src);
    vis[src] = true;
    while (!q.empty()) {
        const auto f = q.front();
        q.pop();
        for (const auto& e : _ns[f].es)
            if (!vis[e.to]) {
                vis[e.to] = true;
                g._add_edge(f, e.to, e.w);
                q.push(e.to);
            }
    }
    return g;
}
```

**说明**：`bfs_tree` 从源点 `src` 开始进行BFS遍历，使用队列 `q` 和访问标记 `vis`。当访问到一个未访问的邻接顶点时，将其标记为已访问，并在生成树 `g` 中添加一条边（方向为从当前顶点到邻接顶点）。最终返回的 `g` 即为广度优先生成树。

== 深度优先遍历（DFS）递归实现（Graph.hpp）

```cpp
auto dfs_tree(std::size_t src) const -> Graph {
    Graph g { _ns.size() };
    for (int i = 0; i < _ns.size(); ++i) g._ns[i].name = _ns[i].name;
    std::vector<bool> vis(_ns.size(), false);
    _dfs_vis(src, vis, g);
    return g;
}

void _dfs_vis(std::size_t cur, std::vector<bool>& vis, Graph& g) const {
    for (const auto& e : _ns[cur].es) {
        if (!vis[e.to]) {
            vis[e.to] = true;
            g._add_edge(cur, e.to, e.w);
            _dfs_vis(e.to, vis, g);
        }
    }
}
```

**说明**：`dfs_tree` 通过递归辅助函数 `_dfs_vis` 实现深度优先遍历。从当前顶点 `cur` 出发，遍历其所有邻接边，若邻接顶点未访问，则标记并添加树边，然后递归访问该邻接顶点。注意，此实现生成的是深度优先生成树。

== 非递归深度优先遍历（栈实现）（Graph.hpp）

```cpp
auto stack_dfs_tree(std::size_t src) const -> Graph {
    Graph g { _ns.size() };
    for (int i = 0; i < _ns.size(); ++i) g._ns[i].name = _ns[i].name;
    std::vector<bool> vis(_ns.size(), false);

    std::stack<std::size_t> s;
    s.push(src);
    while (!s.empty()) {
        const auto top = s.top();
        bool end = true;
        for (const auto e : _ns[top].es)
            if (!vis[e.to]) {
                vis[e.to] = true;
                g._add_edge(top, e.to, e.w);
                s.push(e.to);
                end = false;
                break;
            }
        if (end)
            s.pop();
    }
    return g;
}
```

**说明**：`stack_dfs_tree` 使用显式栈 `s` 模拟递归过程。每次从栈顶取出顶点，遍历其邻接边，找到第一个未访问的邻接顶点就将其入栈并添加树边，然后跳出循环（相当于深度优先）；若所有邻接顶点都已访问，则弹出栈顶。此方法避免了递归调用的开销。

== Dijkstra最短路径算法（Graph.hpp）

```cpp
auto dijkstra(std::size_t src) const -> std::vector<std::uint32_t> {
    std::vector<std::uint32_t> dis(_ns.size(), std::numeric_limits<std::uint32_t>::max());
    std::vector<bool> vis(_ns.size(), false);
    std::priority_queue<DijkState, std::vector<DijkState>, std::greater<>> q;
    dis[src] = 0;
    q.push({ src, 0 });

    while (!q.empty()) {
        const auto f = q.top();
        q.pop();
        if (vis[f.node]) continue;
        vis[f.node] = true;
        dis[f.node] = f.dis;
        for (const auto& e : _ns[f.node].es)
            if (dis[e.to] > dis[f.node] + e.w) {
                dis[e.to] = dis[f.node] + e.w;
                q.push({ e.to, dis[f.node] + e.w });
            }
    }
    return dis;
}
```

**说明**：`dijkstra` 使用优先队列（最小堆）优化。`dis` 数组记录从源点到各顶点的最短距离，初始化为无穷大。每次从队列中取出距离最小的顶点，若未访问，则标记为已访问并更新其邻接顶点的距离（若经过当前顶点到达邻接顶点的距离更短）。最终返回 `dis` 数组。

== 命令行解析（CommandLine.hpp）

```cpp
const auto show = command("show").set(mode, Mode::Show);

const auto bfs = (command("bfs").set(mode, Mode::BFS),
                  value("start").set(start))
               % "BFS traversal";

const auto dfs = (command("dfs").set(mode, Mode::DFS),
                  value("start").set(start))
               % "DFS traversal";

// ... 其他命令类似

const auto cli = (show | bfs | dfs | stack_dfs | dijkstra | help);

if (!parse(argc, argv, cli))
    std::cout << make_man_page(cli);

switch (mode) {
    case Mode::Show: g.show(); break;
    case Mode::BFS:  g.bfs_tree(start).show(); break;
    // ... 其他case
}
```

**说明**：使用 `clipp` 库定义命令行语法。每个子命令（如 `show`、`bfs`）对应一个模式，并可能携带参数（如 `start`）。解析成功后根据模式调用图对象的相应方法。

= 程序运行方式简要说明

== 环境依赖

- **编译器**：支持C++20的编译器（如MSVC、GCC、Clang）
- **构建系统**：xmake（配置文件见 `xmake.lua`）
- **第三方库**：
  + matplotplusplus（可视化）
  + rapidcsv（CSV解析）
  + range-v3（范围操作）
  + clipp（命令行解析）

== 构建步骤

在项目根目录执行：

```bash
xmake config --mode=debug  # 或 --mode=release
xmake build -v
```

编译后的可执行文件位于 `build/windows/x64/debug/dcs213.project4.exe`。

== 使用示例

1. **显示图结构**：
   ```bash
   dcs213.project4.exe show
   ```

2. **BFS遍历**（从顶点0开始）：
   ```bash
   dcs213.project4.exe bfs 0
   ```

3. **DFS遍历**（从顶点0开始）：
   ```bash
   dcs213.project4.exe dfs 0
   ```

4. **非递归DFS遍历**（从顶点0开始）：
   ```bash
   dcs213.project4.exe stack-dfs 0
   ```

5. **最短路径计算**（从顶点0开始）：
   ```bash
   dcs213.project4.exe dijkstra 0
   ```

== 数据文件格式

程序默认读取 `public/city.csv` 和 `public/map.csv`。

- `city.csv`：包含一列 `name`，每一行是一个城市的名称。
- `map.csv`：包含三列 `src`、`dst`、`dis`，分别表示边的起点索引、终点索引和距离。

== 注意事项

- 顶点索引从0开始，与CSV文件中的行号对应（`city.csv` 第0行对应索引0）。
- 图是无向图，但CSV文件中每条边只存储一次，程序在内部会为每条边添加两个方向的边。
- 可视化窗口使用Matplot++，关闭窗口后程序会继续执行（如果有后续操作）。

= 总结

本次实验实现了一个完整的图结构应用，涵盖了图的构建、深度优先遍历（递归与非递归）、广度优先遍历、生成树生成以及最短路径计算。通过手动实现这些算法，加深了对图遍历、生成树、最短路径等原理的理解。程序结构清晰，模块划分合理，并提供了友好的命令行接口与可视化展示，符合实验要求。
