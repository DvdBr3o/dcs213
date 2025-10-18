#import "style.typ": *

#set text(lang: "cn")

#show: dvdbr3otypst.with(
  title: [DCS213 Project1 \ 实验报告],
  subtitle: [],
  author: [代骏泽],
  xno: [24363012],
  bib: bibliography("reference.bib"),
)

#pagebreak()

= 功能设计

本项目实现了 常数表达式运算 和 (一元)变量表达式(多项式)化简. 并将两个功能整合到了同一个图形界面中.

#figure(caption: [运行示例])[
	#grid(
		columns: 2,
		align: center,
		gutter: 1em,
		image("public/res1_input.png"),
		image("public/res1_output.png"),
	)
]

== 常数表达式运算

=== 运算符支持

#figure(caption: [常量表达式运算符])[
  #table(
    columns: 2,
    align: horizon + left,
    [运算符], [功能],
    [`+`], [算术加/单目加],
    [`-`], [算术减/单目减],
    [`*`], [算术乘],
    [`/`], [算术除],
    [`^`], [指数运算],
    [`ln`], [对数运算],
    [`'`], [求导]
  )
]

=== 优先级支持

不加括号可以自动识别优先级，也可以通过括号改变运算顺序

#figure(caption: [优先级示例])[
  #table(
    columns: 3,
    align: horizon + left,
    [输入], [运算顺序], [结果],
    [`1+2*3^4`], [`1+(2*(3^4))`], [`163`],
    [`(1+2)*3^4`], [`(1+2)*(3^4)`], [`84`],
  )
]

=== 结合性支持

支持识别应用同优先级运算符的结合性.

#figure(caption: [结合性示例])[
  #table(
    columns: 3,
    align: horizon + left,
    [输入], [运算顺序], [结果],
    [`1+2+3`], [`(1+2)+3`], [`6`],
    [`2^3^2`], [`2^(3^2)`], [`512`],
  )
]

== 变量表达式化简

#figure(caption: [变量表达式运算符])[
  #table(
    columns: 3,
    align: horizon + left,
    [运算符], [功能], [备注],
    [`+`], [算术加/单目加], [],
    [`-`], [算术减/单目减], [],
    [`*`], [算术乘], [],
    [`/`], [算术除], [],
    [`^`], [指数运算], [],
    [`ln`], [对数运算], [],
    [`'`], [求导], [只支持对多项式求导],
    [`$`], [代入值], [用法为 `Expr $ val`],
  )
]

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

具有丰富特性:

1. 明暗主题切换

2. 响应式布局

= 原理解析

本项目由于采用 webview 作为前端用户界面，所以前后端分界比较分明

#figure(caption: "框架图")[#image("public/dcs213p1_1.drawio.png")]

== 分词器 Lexer

分词器接受来自 UI 的 DisplayInput 或者命令行输入的字符串，将其分成不可再分的 `Token`, 形成一个 `TokenStream`. 而 `TokenStream` 本质上就是一个线性表结构.

=== Token 设计

`Token` 大体设计如下

#figure(caption: [Token 设计])[
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

课上老师介绍了用栈实现中缀表达式的运算，然而课上的示例只涉及 $+$, $-$, $times$, $div$ 四种运算. 而面对更多运算符，以及不同运算符中的优先级、结合性、前中后缀等性质单纯用栈匹配就比较乏力了.

于是我们面临以下两个问题:

1. 单纯的栈 `std::stack` 数据结构表达力有限，难维护

2. 需要设计一个衡量并统筹运算符的优先级、结合性、前中后缀等性质的机制

=== 问题分析

==== 问题一: 栈表达力

事实上，函数递归可以用来模拟栈或者被栈模拟. 因为函数调用本质上是对函数帧的栈结构维护，我们可以建立起函数调用与栈概念的一一映射:

- 栈元素 $equiv$ 函数局部寄存器/栈变量状态 aka. 函数帧

- 栈压入 $equiv$ 函数调用

- 栈弹出 $equiv$ 函数返回

而函数递归等价于每个"栈元素"的编译期布局完全一致 (函数帧布局完全一致). 鉴于函数递归与栈的相似性，本项目决定用递归函数替代栈来进行运算表达式匹配. 递归函数替代栈还带来了 bonus:

1. 更清晰、更可维护的逻辑. 函数帧布局是完全隐式的，而控制流更清晰.

2. 更快的速度. 手动维护栈结构或者 `std::stack` 都是堆分配. 而函数帧是完全在寄存器和栈上分配的, 无论是分配速度还是局部连续性都优于堆分配，带来更快的速度. 代价是有限的匹配深度，但鉴于 clang 和 rustc 都是手写递归下降递归函数也没爆栈过，所以完全不必担心.

==== 问题二: 运算符匹配机制 <my_pratt_parser>

首先可以用数值表示一个运算符的优先级，通过比较数值来确定两个运算符的优先级优劣. 添加新运算符时就无需构造像题目要求里给出的表.

其次对于结合性问题，类似 $+$，$times$ 这样的运算符遵循左结合：

$
  1+2+3 => (1 + 2) + 3
$

$
  1 times 2 times 3 => (1 times 2) times 3
$

而有时我们又有一些右结合运算符，如指数运算 `^`:

$
  2^3^4 => 2^((3^4))
$

于是很容易想到对每个运算符构造类似于 "左优先级" 和 "右优先级" 的数值，通过同一运算符不同的左右优先级实现左结合和右结合.

最后对于前缀运算符和后缀运算符我们还可以添加 "前缀优先级" 和 "后缀优先级" 的数值. 对于没有前后缀语义的运算符，前后缀优先级设为 $-1$ 表示不可用即可.

我们不妨给这些优先级数值取名为 *Bindpower*.

#align(center)[
  #table(
    columns: 3,
    align: horizon + left,
    [术语], [全称], [定义],
    [PBP], [Prefix Bindpower], [前缀结合力],
    [LBP], [Infix Left Bindpower], [中缀左结合力],
    [RBP], [Infix Right Bindpower], [中缀右结合力],
    [SBP], [Suffix Bindpower], [后缀结合力],
  )
]

可以很形象地直观理解，结合力大的运算符对操作数吸引力大，更优先运算. 在遍历 `TokenStream` 时不断对比操作数左右的运算符结合力，若右运算符结合力大则该操作数与 `TokenStream` 右边的剩余 Token 形成项，递归匹配；反之左运算符结合力大则可直接确定与左边形成 LHS.

```
      a+b*c

        ↓

a            b    c
    3     4    5 6
       +        *  

        ↓

a      +    (b  * c)
```

最后通过匹配可以构造一棵语法树 AST.

我们不妨给我们这个做梦梦到的算法起名为 Pratt Parser @simple_but_powerful_pratt_parsing.

=== 语法树设计

#figure(caption: [AST Node 设计])[
	#align(center)[
		#table(
			columns: 3,
			align: horizon + left,
			[类型], [内涵], [结构],
			[`BinOpExpr`], [二元操作符表达式], [```cpp
			struct BinOpExpr {
			  lex::Operator         op;
			  std::unique_ptr<Expr> lhs;
			  std::unique_ptr<Expr> rhs;
			};
			```],
			[`UnaryOpExpr`], [一元操作符表达式], [```cpp
			struct UnaryOpExpr {
			  lex::Operator          op;
			  std::unique_ptr<Expr> oper;
			};
			```],
			[`Number`], [数字字面量], [```cpp
			struct Number {
			  double val;
			};
			```],
			[`Variable`], [变量], [```cpp
			struct Variable {};
			```]
		)
	]
]

仍然选择使用类似 uniform / Rust enum 的 SumType `std::variant` 实现语法树节点的多态.

```cpp
struct Expr : public std::variant<BinOpExpr, UnaryOpExpr, Number, Variable> {
		inline friend auto to_string(const Expr& expr) -> std::string { return expr.to_string(); }

		[[nodiscard]] auto to_string() const -> std::string {
			return std::visit([](const auto& expr) { return expr.to_string(); }, *this);
		}

		template<typename T>
		auto get_if() -> T* {
			return std::get_if<T>(this);
		}

		template<typename T>
		auto get_if() const -> const T* {
			return std::get_if<T>(this);
		}

		template<typename T>
		auto is() const -> bool {
			return std::holds_alternative<T>(*this);
		}
};

```

=== 匹配算法设计

==== BindPower 表生成算法设计

设计运算符 BindPower 时，如果手动指定会比较麻烦，而且插入新运算符时又要重新指定，非常麻烦. 为节省头发，我们可以设计一个编译期的 BindPower 表生成算法. 

只需要声明一条结合性+优先级链就行，通过遍历将其转化为一张表，不可用的项设为 $-1$. 由于是编译期生成，所以是零开销.

详细实现见 `./src/BindPower.hpp`.

```cpp
inline static constexpr auto operator_bindpower_factory() {
	using lex::Operator;
	using namespace asso;

	return build(
		pre(Operator::Plus)			     // unary +
		>= pre(Operator::Minus)		   // unary -
		> suf(Operator::Derivative)	 // '
		> right(Operator::Exponent)	 // ^
		> pre(Operator::Ln)			     // ln
		> left(Operator::Multiply)	 // *
		>= left(Operator::Devide)	   // /
		> left(Operator::Plus)		   // +
		>= left(Operator::Minus)	   // -
		> left(Operator::When)		   // $
	);
}

inline static constexpr auto bindpower = operator_bindpower_factory();
```

最后生成的 BindPower 表如下:

#figure(caption: [BindPower 表])[
	#align(center)[
		#table(
			columns: 5,
			align: horizon + left,
			[运算符], [PBP], [LBP], [RBP], [SBP],
			[+],  [11], [3], [4], [-1],
			[-],  [11], [3], [4], [-1],
			[\* ],  [-1], [5], [6], [-1],
			[\/],  [-1], [5], [6], [-1],
			[^],  [-1], [9], [8], [-1],
			[ln],  [7], [-1], [-1], [-1],
			[(],  [-1], [-1], [-1], [-1],
			[)],  [-1], [-1], [-1], [-1],
			['],  [-1], [-1], [-1], [10],
			[\$],  [-1], [1], [2], [-1],
		)
	]
]

==== 运算符匹配算法设计

如我们 #link(<my_pratt_parser>)[梦到的运算符匹配算法] 所述:

```cpp
	inline static auto parse(lex::TokenStream::View& ts, std::size_t min_bp)
		-> tl::expected<Expr, ParseError> {
		if (const auto tok = ts.bump()) {
			Expr lhs;

			if (const auto op = tok->get_if<lex::Operator>()) {
				if (*op == lex::Operator::LParen) {
					if (auto prs = parse(ts, 0)) {
						if (ts.expect({ lex::Operator::RParen }))
							lhs = *std::move(prs);
						else
							return make_error(Errors::RParenMiss {});
					}
				} else if (const auto pbp = bindpower[*op].pbp; pbp > 0) {
					if (auto&& rhs = parse(ts, pbp))
						lhs = {
							UnaryOpExpr {
										 .op		 = *op,
										 .operand = std::make_unique<Expr>(*std::move(rhs)),
										 }
						};
				}
			} else if (const auto num = tok->get_if<lex::Number>()) {
				lhs = { Number { num->value } };
			} else if (const auto con = tok->get_if<lex::Constant>()) {
				lhs = { Number { val(*con) } };
			} else if (const auto var = tok->get_if<lex::Variable>()) {
				lhs = { Variable {} };
			}

			while (const auto tok = ts.peek()) {
				if (const auto op = tok->get_if<lex::Operator>()) {
					// suffix
					if (const auto sbp = bindpower[*op].sbp; sbp > 0) {
						if (sbp < min_bp)
							break;

						ts.bump();

						lhs = {
							UnaryOpExpr {
										 .op		 = *op,
										 .operand = std::make_unique<Expr>(std::move(lhs)),
										 }
						};

						continue;
					}

					// infix
					if (const auto [lbp, rbp] = bindpower[*op].infix; lbp > 0 && rbp > 0) {
						if (lbp < min_bp)
							break;
						ts.bump();
						if (auto rhs = parse(ts, rbp)) {
							lhs = {
								BinOpExpr {
										   .op	 = *op,
										   .lhs = std::make_unique<Expr>(std::move(lhs)),
										   .rhs = std::make_unique<Expr>(std::move(*rhs)),
										   }
							};
						} else
							return make_error(Errors::RhsMiss {
								.op	 = *op,
								.lhs = std::make_unique<Expr>(std::move(lhs)),
							});

						continue;
					}
				}

				break;
			}

			return std::move(lhs);
		}

		return make_error(Errors::NotMatched {});
	}
```

== 运算器 Evaluator

运算器接受来自解析器的 `AST`，经过一系列代数系统下的化简规则 (如结合律、交换律、分配律)，将 `AST` 进行化简，并将化简后的 `AST` format 成字符串形式，返回给用户.

我希望将 常数表达式运算 和 变量表达式化简 合并到同一个交互逻辑中。事实上，常数表达式可以视为 变量表达式化简 中的 常数折叠 操作，这样就实现了两种功能的统一. 但考虑到从头构建一套 CAS 系统的复杂性，本项目只基于多项式 `TermList` 概念进行化简. 这样避免了代数系统中考虑零元、幺元、结合律、交换律以及类似 $cos^2 x+sin^2 x=1$ 等特殊定律的复杂性.

主要内容在 `./src/Evalutor.hpp` 下.

=== 常量折叠

核心实现在于 `dcs213::p1::evaluate::eval_con` 函数. 若子 AST 可以被常量折叠，则返回折叠结果；否则返回 `std::nullopt`.

只需要递归遍历 AST，根据 `BinOpExpr`、`UnaryOpExpr` 的运算符 `op` 以及操作数返回运算结果即可.

```cpp
	inline static auto eval_con(const parse::Expr& expr) -> std::optional<double> {
		// std::cout << std::format("parsing con: {}\n", expr.to_string());
		if (const auto binop = expr.get_if<parse::BinOpExpr>()) {
			if (const auto lhs = eval_con(*binop->lhs))
				if (const auto rhs = eval_con(*binop->rhs))
					switch (binop->op) {
						case lex::Operator::Plus: return *lhs + *rhs;
						case lex::Operator::Minus: return *lhs - *rhs;
						case lex::Operator::Multiply: return *lhs * *rhs;
						case lex::Operator::Devide: return *lhs / *rhs;
						case lex::Operator::Exponent: return std::pow(*lhs, *rhs);
						default: return std::nullopt;
					}
		} else if (const auto uop = expr.get_if<parse::UnaryOpExpr>()) {
			if (const auto oper = eval_con(*uop->operand))
				switch (uop->op) {
					case lex::Operator::Plus: return *oper;
					case lex::Operator::Minus: return -*oper;
					case lex::Operator::Derivative: return 0;
					case lex::Operator::Ln: return std::log(*oper);
					default: return std::nullopt;
				}
		} else if (const auto num = expr.get_if<parse::Number>())
			return num->val;

		return std::nullopt;
	}
```

=== 多项式化简

==== 多项式结构设计

多项式 `TermList` 是关于项 `Term` 的线性表. `Term` 只记录项的系数和指数.

```cpp
	struct Term {
		double coef = 1.;  // coefficient
		double expo = 1.;  // exponent
};
```

==== 多项式运算

递归地遍历 AST，先匹配运算模式 (加减、乘、求导)，再匹配多项式.



===== 多项式加减

在保证两个 `TermList` 的元素都有序的情况下可以通过双指针实现 $O(n)$ 的多项式加减.

===== 多项式乘积

保证有序情况下朴素做法也最多只能实现 $O(n^3)$ 的做法. 所以实际实现采用了哈希表实现 $O(n^2log n)$.

```cpp
class TermList : public std::vector<Term> {
public:
  // ...
  inline friend auto operator*(const TermList& lhs, const TermList& rhs) -> TermList {
			TermList						   res;

			std::unordered_map<double, double> terms;

			for (const auto& [c1, e1] : lhs)
				for (const auto& [c2, e2] : rhs) {
					const auto e = e1 + e2;
					if (terms.find(e) != terms.end())
						terms[e] += c1 * c2;
					else
						terms[e] = c1 * c2;
				}

			res.reserve(terms.size());

			for (const auto& [e, c] : terms) res.emplace_back(c, e);

			res._sort_self();

			return res;	 // nrvo
		}

};
```

为了遵循项目要求中尽量不要使用 STL 的准则，此处手动实现了 `TermList` 的排序，采用了运用分治思想的归并排序

```cpp
class TermList : public std::vector<Term> {
  // ...
private:
		auto _sort_self() -> void { _sort_self(0, size()); }

		auto _sort_self(std::size_t l, std::size_t r) -> void {
			if (l == r - 1)
				return;

			Term*	   terms = new Term[r - l];

			const auto m	 = (l + r) >> 1;

			_sort_self(l, m);
			_sort_self(m, r);

			auto		lp	 = l;
			auto		rp	 = m;
			auto		cnt	 = 0;
			const auto& self = *this;

			while (lp < m && rp < r)
				if (self[lp].expo < self[rp].expo)
					terms[cnt++] = self[lp++];
				else
					terms[cnt++] = self[rp++];

			while (lp < m) terms[cnt++] = self[lp++];
			while (rp < r) terms[cnt++] = self[rp++];

			memcpy(data() + l, terms, (r - l) * sizeof(Term));

			delete[] terms;
		}

};
```

===== 多项式求导

这里又体现了从 CAS 偷懒成多项式的好处，我们只需考虑 $(x^a)'=a x^(a-1)$ 即可. 对 `TermList` 的每个 `Term` 元素进行 $(x^a)'=a x^(a-1)$ 的求导运算即可.

== 用户界面 UI

本项目采用了 webview 作为前端用户界面，带来以下优点

- *更好的开发体验*: 相比于不能热更新且许可证非常受限的 Qt，webview 基于 web 的技术栈显然有更好的开发体验。另外 C++ 与 js 之间的通信也非常方便，对比 flutter 来说的 with C++ (by cffi & `extern "C"`) 体验更好

- *更好的可移植性*: webview 天生具有优越的跨平台性，相比于局限于 Windows 平台的 MFC/UWP/WinUI 更有优势

- *更好的用户界面*: 虽然本项目只用了 html + css + js 的 vanilla web 技术栈开发，但是 webview 理论上适配所有支持 ssg 乃至可以 self host 的前端技术栈 (webview nagivate 到 self host 暴露的端口即可)，这些都能比 Qt, MFC 等自渲染方案带来更好的生态以及更美的外观

- *更小的发行体积*: 相比嵌入浏览器引擎的 cef 方案，发行体积更小，可以媲美 Qt, MFC 等方案的发行体积 (毕竟真正的体积大头 webview runtime 是用系统自带的, 分发程序只包含计算器逻辑以及硬编码的 html 文本)


本项目的用户界面主要包括两个部分: DisplayBox 和 ButtonBox.

DisplayBox 主要用于接受用户输入，并显示计算结果.

ButtonBox 为用户提供了键盘输入外的点击输入方式，提供了一些快捷功能 (比如一键清空，手动切换亮暗主题等)，并丰富了界面.

当用户在 DisplayBox 输入回车或者点击 "`=`" 按钮时，前端把 DisplayBox 的文本内容发送给后端，并将计算结果输出显示到 DisplayBox.

#info[
  前端与后端的通信，在使用静态 html + js 时应该是使用的 IPC，也即本项目的方案. 

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
