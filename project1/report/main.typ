#import "style.typ": *

#show: dvdbr3otypst.with(
  title: [DCS213 Project1 \ 实验报告],
  subtitle: [],
  author: [代骏泽],
  xno: [24363012],
  bib: bibliography("reference.bib"),
)

#pagebreak()

= 功能设计

== 概论

@simple_but_powerful_pratt_parsing

== 常数表达式运算



== 变量表达式化简

== 用户界面

#figure(caption: "用户界面")[
	#grid(
		columns: 2,
		align: center,
		gutter: 1em,
		image("public/calc_dark.png"),
		image("public/calc_light.png"),
	)
]

= 原理解析

== 概论

本项目由于采用 webview 作为前端用户界面，所以前后端分界比较分明

#figure(caption: "框架图")[#image("public/dcs213p1_1.drawio.png")]

== 分词器 Lexer

分词器接受来自 UI 的 DisplayInput 或者命令行输入的字符串，将其分成不可再分的 `Token`, 形成一个 `TokenStream`. 而 `TokenStream` 本质上就是一个线性表结构.

=== Token 设计

`Token` 大体设计如下

#align(center)[
  #table(
    columns: (auto, auto, auto),
    align: horizon + left,
    table.header([种类], [值], [含义]),
    [`Number`], [$bb(R)$ in `double`], [实数立即数],
    [`Operator`], [`+`], [算术加],
    [`Operator`], [`-`], [算术减/单目减],
    [`Operator`], [`*`], [算术乘],
    [`Operator`], [`/`], [算术除],
    [`Operator`], [`^`], [指数运算],
    [`Operator`], [`ln`], [自然对数运算],
    [`Operator`], [`'`], [变量表达式求导运算],
    [`Constant`], [$e$], [自然常数],
    [`Constant`], [$pi$], [圆周率],
    [`Variant`], [$x$], [变量],
  )
]

实际代码用 `std::variant` 实现 `Token`，类似于 Rust 的 `enum`.

```cpp
struct Token {
		using Kind = std::variant<Number, Operator, Constant, Variable>;

		Kind			   token;
		bool			   conj = false;

    // ...
}
```

每一种 `Token` 种类都通过一个 `lex_xxx()` 函数尝试匹配，匹配成功就向 `TokenStream` 添加 `Token`，否则就进行下一个匹配. 如果所有匹配都匹配失败，则通过失败值给用户提示异常.

#info[
  考虑到单种匹配函数匹配失败在整个 tokenize 过程是十分频繁的 ( 有很大一部分是 `LexError::NotMatched` )，而 C++ exception 由于栈展开在 unhappy path 上的开销十分昂贵，所以本项目使用 C++23 的 monadic 异常 `std::expected` 作为匹配失败的异常方案. 这样性能上基本 0 overhead 而且实现了模块化.
]

=== 分词算法设计

==== 规则匹配函数

所有的 `lex_xxx()` 匹配规则函数都符合 `auto lex_xxx(std::string_view) -> LexResult` 签名，而 `LexResult := tl::expected<LexSuccess, LexError>`. 其中 `LexSuccess` 记录匹配到的 `Token` 信息以及剩余待匹配文本的视图，`LexError` 则为已知可能错误的 `variant`.

所有的匹配函数基本都是 look ahead 1/2 token. 所以总体时间复杂度可控，大概在 $O(n)$.

以 `lex_operator` 为例

```cpp
inline static auto lex_operator(std::string_view script) -> LexResult {
	if (script.size() >= 2 && script.substr(0, 2) == "ln") {
		return LexSuccess::make(Operator::Ln, script.substr(2));
	} else if (script.size() >= 1) {
		Cursor	 cursor = script;
		Operator oper;
		if (const auto op = cursor.bump()) {
			switch (op) {
				case '+': oper = Operator::Plus; break;
				case '-': oper = Operator::Minus; break;
				case '*': oper = Operator::Multiply; break;
				case '/': oper = Operator::Devide; break;
				case '^': oper = Operator::Exponent; break;
				case '(': oper = Operator::LParen; break;
				case ')': oper = Operator::RParen; break;
				case '\'': oper = Operator::Derivative; break;
				default: return tl::make_unexpected(LexErrors::NotMatched {});
			}
			return LexSuccess::make(oper, script.substr(1));
		}
	}
	return tl::make_unexpected(LexErrors::NotMatched {});
}
```

==== 规则聚合

最后本项目设计了一个 `handle_lex` 以及 `lex` 实现各种规则匹配的聚合.

`handle_lex` 实现了编译期内联的、零开销的任意规则函数聚合.

```cpp
template<std::invocable<std::string_view>... Ts>
struct handle_lex : std::tuple<Ts...> {
	constexpr handle_lex(Ts&&... ts) : std::tuple<Ts...> { std::forward<Ts>(ts)... } {}

	auto operator()(std::string_view script) const -> LexResult {
		return apply(script, std::make_index_sequence<sizeof...(Ts)>());
	}

	template<std::size_t I0, std::size_t... Is>
	auto apply(std::string_view script, std::index_sequence<I0, Is...>) const -> LexResult {
		auto res = std::get<I0>(*this)(script);
		if (res)
			return std::move(res).value();
		else if(!std::holds_alternative<LexErrors::NotMatched>(res.error()))
			return tl::make_unexpected(std::move(res).error());
		else
			return apply(script, std::index_sequence<Is...>());
	}

	auto apply(std::string_view script, std::index_sequence<>) const -> LexResult {
		return tl::make_unexpected(LexErrors::NotMatched {});
	}
};
```

而 `lex` 函数负责将匹配的 `Token` 组织成一个线性表 `TokenStream`.

```cpp
inline static auto lex(std::string_view script) -> tl::expected<TokenStream, LexError> {
	static constexpr auto lex_handler = handle_lex {
		&lex_number,
		&lex_operator,
		&lex_constant,
		&lex_variable,
	};

	TokenStream ts;

	while (true) {
		auto res = lex_handler(script);
		if (res)
			ts.emplace_back(std::move(res)->tok);
		else if (!std::holds_alternative<LexErrors::NotMatched>(res.error()))
			return tl::make_unexpected(std::mov(res).error());

		script = res->rest;

		if (script.empty())
			break;
	}

	return ts;	// nrvo
}
```

== 解析器 Parser

解析器接受来自分词器的 `TokenStream`，并将其根据运算符的前缀/后缀性质、左右结合性、优先级，构造成一个忠于原始表达式的符号树 `AST`

== 运算器 Evaluator

运算器接受来自解析器的 `AST`，经过一系列代数系统下的化简规则 (如结合律、交换律、分配律)，将 `AST` 进行化简，并将化简后的 `AST` format 成字符串形式，返回给用户.

我希望将 常数表达式运算 和 变量表达式化简 合并到同一个交互逻辑中。事实上，常数表达式可以视为 变量表达式化简 中的 常数折叠 操作，这样就实现了两种功能的统一.

== 用户界面 UI

本项目采用了 webview 作为前端用户界面，带来以下优点

- *更好的开发体验*: 相比于不能热更新且许可证非常受限的 Qt，webview 基于 web 的技术栈显然有更好的开发体验。另外 C++ 与 js 之间的通信也非常方便，对比 flutter 来说的 with C++ 体验更好

- *更好的可移植性*: webview 天生具有优越的跨平台性，相比于局限于 Windows 平台的 MFC/UWP/WinUI 更有优势

- *更好的用户界面*: 虽然本项目只用了 html + css + js 的 vanilla web 技术栈开发，但是 webview 理论上适配所有支持 ssg 乃至可以 self host 的前端技术栈 (webview nagivate 到 self host 暴露的端口即可)，这些都能比 Qt, MFC 等自渲染方案带来更好的生态以及更美的外观

- *更小的发行体积*: 相比嵌入浏览器引擎的 cef 方案，发行体积更小，可以媲美 Qt, MFC 等方案的发行体积 (毕竟真正的体积大头 webview runtime 是用系统自带的)


本项目的用户界面主要包括两个部分: DisplayBox 和 ButtonBox.

DisplayBox 主要用于接受用户输入，并显示计算结果.

ButtonBox 为用户提供了键盘输入外的点击输入方式，提供了一些快捷功能 (比如一键清空，手动切换亮暗主题等)，并丰富了界面.

当用户在 DisplayBox 输入回车或者点击 "`=`" 按钮时，前端把 DisplayBox 的文本内容发送给后端，并将计算结果输出显示到 DisplayBox.

#info[
  前端与后端的通信，在使用静态 html + js 时应该是使用的 IPC，也即本项目的方案. 

  \
  而若通过 webview 的 `navigate` 方法连接一个本地的 self host 前端 server，则也可以通过 localhost 网络通信进行通信. 但无论哪种方案都是通过传递 json 字符串进行，有一定开销，而此开销在本项目中可以接受.
]

= 复现指引

== 环境准备

=== Windows

```bash
scoop install xmake
scoop install clang
scoop install vcpkg
vcpkg install webview2
```

至于 scoop 如何安装见 scoop 官网

=== Linux

以 archlinux with yay 为例

```bash
yay -S xmake clang vcpkg gtk webkit2gtk
```

#info([
  `gtk` & `webkit2gtk` 是 `webview` 库在 linux 下的必要依赖，AUR 库中是这两个名字，至于在 `apt` 等库中的名字则需自行查询
])

== 构建运行

xmake is all you need.

```bash
xmake f -m release -y   # configure & install dependencies
xmake b dcs213.project1 # build, in fact this is unnecessary since 
                        # the behaviour of latest xmake is always build 
                        # before run if target not built.
xmake r dcs213.project1 # run
```

#align(right)[#strike([fuck cmake.])]
