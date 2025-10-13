#pragma once

#include "String.hpp"
#include "Utils.hpp"

#include <tl/expected.hpp>

#include <numbers>
#include <optional>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace dcs213::p1::lex {
	inline static constexpr auto is_number(char c) -> bool;

	inline static constexpr auto is_space(char c) -> bool;

	inline static constexpr auto is_space(std::string_view script) -> bool;

	inline static constexpr auto trim_space(std::string_view script) -> std::string_view;

	struct Token;

	struct Number {
		double						 value;

		inline friend constexpr auto operator==(const Number& lhs, const Number& rhs) -> bool {
			return lhs.value == rhs.value;
		}
	};

	enum class Operator : std::uint32_t {
		Plus,		 // +
		Minus,		 // -
		Multiply,	 // *
		Devide,		 // /
		Exponent,	 // ^
		Ln,			 // ln
		LParen,		 // (
		RParen,		 // )
		Derivative,	 // '
	};

	inline static constexpr auto to_string(Operator op) -> std::string {
		switch (op) {
			case Operator::Plus: return "+";
			case Operator::Minus: return "-";
			case Operator::Multiply: return "*";
			case Operator::Devide: return "/";
			case Operator::Exponent: return "^";
			case Operator::Ln: return "ln";
			case Operator::LParen: return "(";
			case Operator::RParen: return ")";
			case Operator::Derivative: return "'";
		}
	}

	enum class Constant : std::uint32_t {
		E,	 // e
		Pi,	 // pi
	};

	inline static constexpr auto val(Constant constant) -> double {
		switch (constant) {
			case Constant::E: return std::numbers::e;
			case Constant::Pi: return std::numbers::pi;
		}
	}

	inline static constexpr auto to_string(Constant c) -> std::string {
		switch (c) {
			case Constant::E: return "e";
			case Constant::Pi: return "pi";
		}
	}

	struct Variable {};

	struct Token {
		using Kind = std::variant<Number, Operator, Constant, Variable>;

		Kind			   token;
		bool			   conj = false;

		[[nodiscard]] auto to_string() const -> std::string {
			std::string str;
			return std::visit(
				overload { [&](const Number& tok) {
							  return std::format("NUMBER({})", std::get<Number>(token).value);
						  },
						   [&](const Operator& tok) {
							   switch (tok) {
								   case Operator::Plus: return std::format("OPERATOR(+)");
								   case Operator::Minus: return std::format("OPERATOR(-)");
								   case Operator::Multiply: return std::format("OPERATOR(*)");
								   case Operator::Devide: return std::format("OPERATOR(/)");
								   case Operator::Exponent: return std::format("OPERATOR(^)");
								   case Operator::Ln: return std::format("OPERATOR(ln)");
								   case Operator::LParen: return std::format("OPERATOR(()");
								   case Operator::RParen: return std::format("OPERATOR())");
								   case Operator::Derivative: return std::format("OPERATOR(')");
							   }
						   },
						   [&](const Constant& tok) {
							   switch (tok) {
								   case Constant::E: return std::format("CONSTANT(e)");
								   case Constant::Pi: return std::format("CONSTANT(pi)");
							   }
						   },
						   [&](const Variable& tok) { return std::format("VARIABLE(x)"); } },
				token
			);
		}

		inline friend constexpr auto operator==(const Token& lhs, const Token& rhs) -> bool {
			if (lhs.token.index() != rhs.token.index())
				return false;
			else
				return std::visit(
					overload {
						[&](const Number& tok) { return tok == *std::get_if<Number>(&rhs.token); },
						[&](const Operator& tok) {
							return tok == *std::get_if<Operator>(&rhs.token);
						},
						[&](const Constant& tok) {
							return tok == *std::get_if<Constant>(&rhs.token);
						},
						[&](const Variable& tok) { return true; },
					},
					lhs.token
				);
		}

		constexpr auto&		  operator*() { return token; }

		constexpr const auto& operator*() const { return token; }
	};

	class TokenStreamView;

	class TokenStream : public std::vector<Token> {
	public:
		using vector::vector;
		using View = TokenStreamView;

		constexpr auto view() -> TokenStreamView;
	};

	class TokenStreamView {
	public:
		using iterator = class Iterator {
		public:
			constexpr Iterator(const TokenStream& ts, std::size_t index) : _ts(ts), _index(index) {}

		public:
			constexpr auto operator*() const -> const Token& { return _ts[_index]; }

			constexpr auto operator++() -> Iterator& {
				++_index;
				return *this;
			}

			constexpr auto operator++(int) -> Iterator {
				auto origin = *this;
				++(*this);
				return origin;
			}

			inline friend constexpr auto operator==(const Iterator& lhs, const Iterator& rhs)
				-> bool {
				return &lhs._ts == &rhs._ts && lhs._index == rhs._index;
			}

		private:
			const TokenStream& _ts;
			std::size_t		   _index;
		};

	public:
		constexpr TokenStreamView(const TokenStream& ts) : _ts(ts) {}

	private:
		constexpr TokenStreamView(const TokenStream& ts, std::size_t index, std::size_t end) :
			_ts(ts), _index(index), _end(end) {}

	public:
		constexpr auto subview(std::size_t offset) -> TokenStreamView {
			assert((_index + offset < _end) && "Subview exceeded available range!");
			return { _ts, _index + offset, _end };
		}

		constexpr auto subview(std::size_t offset, std::size_t len) -> TokenStreamView {
			assert(
				(_index + offset < _end && _index + offset + len <= _end)
				&& "Subview exceeded available range!"
			);
			return { _ts, _index + offset, _index + offset + len };
		}

		constexpr auto bump() -> std::optional<Token> { return peek(_index++); }

		constexpr auto peek(std::size_t n = 0) -> std::optional<Token> {
			if (_index + n < _end)
				return _ts[_index + n];
			else
				return std::nullopt;
		}

		constexpr auto expect(const Token& exp) -> bool {
			if (const auto token = bump(); *token == exp)
				return true;
			else
				return false;
		}

		[[nodiscard]] constexpr auto begin() const -> Iterator { return { _ts, _index }; }

		[[nodiscard]] constexpr auto end() const -> Iterator { return { _ts, _end }; }

	private:
		const TokenStream& _ts;
		std::size_t		   _index = 0;
		std::size_t		   _end	  = _ts.size();
	};

	struct LexSuccess {
		Token						 tok;
		std::string_view			 rest;

		inline static constexpr auto make(Token::Kind&& tok, std::string_view rest_raw)
			-> LexSuccess {
			return {
				.tok =
					Token {
						   .token = tok,
						   .conj  = is_space(rest_raw),
						   },
				.rest = trim_space(rest_raw),
			};
		}
	};

	namespace LexErrors {
		struct NotMatched {};

		struct UndefinedIdentifier {
			std::string_view   id;

			[[nodiscard]] auto to_string() const -> std::string {
				return std::format("我不认得 `{}` >_<", id);
			}
		};

		using LexError = std::variant<NotMatched, UndefinedIdentifier>;
	}  // namespace LexErrors

	using LexErrors::LexError;

	using LexResult = tl::expected<LexSuccess, LexError>;

	inline static constexpr auto is_number(char c) -> bool {
		return '0' <= c && c <= '9';
	}

	inline static constexpr auto is_space(char c) -> bool {
		return c == ' ' || c == '\t';
	}

	inline static constexpr auto is_space(std::string_view script) -> bool {
		return script.empty() ? false : is_space(script[0]);
	}

	inline static constexpr auto trim_space(std::string_view script) -> std::string_view {
		Cursor cursor = script;
		while (const auto c = cursor.first())
			if (!is_space(c))
				break;
			else
				cursor.bump();
		return cursor.rest();
	}

	inline static auto lex_unsigned_number(std::string_view script)
		-> tl::expected<std::tuple<double, std::string_view>, LexError> {
		Cursor cursor  = script;
		bool   dotted  = false;
		double val	   = 0.;
		int	   dec_exp = 0;	 // decimal exponent

		if (const auto fst = cursor.first())
			if (!is_number(fst))
				return tl::make_unexpected(LexErrors::NotMatched {});

		while (const auto c = cursor.first()) {
			if (is_number(c))
				if (!dotted)  // integer part
					val = val * 10. + static_cast<double>(c - '0');
				else		  // decimal part
					val = val + std::pow(.1, ++dec_exp) * (c - '0');
			else if (c == '.' && !dotted)
				dotted = true;
			else
				break;
			cursor.bump();
		}

		return std::tuple { val, cursor.rest(0) };
	}

	inline static auto lex_number(std::string_view script) -> LexResult {
		Cursor cursor = script;

		if (const auto c = cursor.first()) {
			double factor = 1.;
			if (is_number(c)) {
				if (const auto res = lex_unsigned_number(script)) {
					const auto [val, rst] = *res;
					return LexSuccess::make(Number { val }, rst);
				}
			} else if (c == '-') {
				if (const auto res = lex_unsigned_number(script.substr(1))) {
					const auto [val, rst] = *res;
					return LexSuccess::make(Number { -val }, rst);
				}
			}
		}

		return tl::make_unexpected(LexErrors::NotMatched {});
	}

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

	inline static auto lex_constant(std::string_view script) -> LexResult {
		Cursor cursor = script;

		if (script.size() >= 2 && script.substr(0, 2) == "pi")
			return LexSuccess::make(Constant::Pi, script.substr(2));
		else if (script.size() >= 1 && script.substr(0, 1) == "e")
			return LexSuccess::make(Constant::E, script.substr(1));

		return tl::make_unexpected(LexErrors::NotMatched {});
	}

	inline static auto lex_variable(std::string_view script) -> LexResult {
		if (script.size() >= 1 && script.substr(0, 1) == "x") {
			return LexSuccess {
				.tok =
					Token {
						   .token = Variable {},
						   .conj  = is_space(script.substr(1)),
						   },
				.rest = trim_space(script.substr(1)),
			};
			return LexSuccess::make(Variable {}, script.substr(1));
		}
		return tl::make_unexpected(LexErrors::NotMatched {});
	}

	template<std::invocable<std::string_view>... Ts>
	struct handle_lex : std::tuple<Ts...> {
		constexpr handle_lex(Ts&&... ts) : std::tuple<Ts...> { std::forward<Ts>(ts)... } {}

		auto operator()(std::string_view script) const -> LexResult {
			return apply(script, std::make_index_sequence<sizeof...(Ts)>());
		}

		template<std::size_t I0, std::size_t... Is>
		[[nodiscard]] auto apply(std::string_view script, std::index_sequence<I0, Is...>) const
			-> LexResult {
			auto res = std::get<I0>(*this)(script);
			if (res)
				return std::move(res).value();
			else if (!std::holds_alternative<LexErrors::NotMatched>(res.error()))
				return tl::make_unexpected(std::move(res).error());
			else
				return apply(script, std::index_sequence<Is...>());
		}

		[[nodiscard]] auto apply(std::string_view script, std::index_sequence<>) const
			-> LexResult {
			return tl::make_unexpected(LexErrors::NotMatched {});
		}
	};

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
				return tl::make_unexpected(std::move(res).error());

			script = res->rest;

			if (script.empty())
				break;
		}

		return ts;	// nrvo
	}
}  // namespace dcs213::p1::lex

namespace dcs213::p1::lex {
	inline constexpr auto TokenStream::view() -> TokenStreamView {
		return { *this };
	}
}  // namespace dcs213::p1::lex