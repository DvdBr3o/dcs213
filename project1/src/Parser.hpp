#pragma once

#include "BindPower.hpp"
#include "Lexer.hpp"

#include <tl/expected.hpp>

#include <memory>
#include <variant>
#include <exception>

namespace dcs213::p1::parse {

	struct Expr;

	struct BinOpExpr {
		lex::Operator				 op;
		std::unique_ptr<Expr>		 lhs;
		std::unique_ptr<Expr>		 rhs;

		inline static constexpr auto to_string(const BinOpExpr& expr) -> std::string;

		[[nodiscard]] constexpr auto to_string() const -> std::string { return to_string(*this); }
	};

	struct UnaryOpExpr {
		lex::Operator				 op;
		std::unique_ptr<Expr>		 operand;

		inline static constexpr auto to_string(const UnaryOpExpr& expr) -> std::string;

		[[nodiscard]] constexpr auto to_string() const -> std::string { return to_string(*this); }
	};

	struct Number {
		double						 val;

		inline static constexpr auto to_string(const Number& num) -> std::string {
			return std::format("{}", num.val);
		}

		[[nodiscard]] constexpr auto to_string() const -> std::string { return to_string(*this); }
	};

	struct Term {
		double coef = 1.;
		double exp	= 1.;
	};

	class TermList {};

	struct Variable {
		inline static constexpr auto to_string(const Variable& expr) -> std::string {
			// return std::format("{} {}", lex::to_string(expr.op),);
			return {};
		}

		[[nodiscard]] constexpr auto to_string() const -> std::string { return to_string(*this); }
	};

	struct Expr : public std::variant<BinOpExpr, UnaryOpExpr, Number, Variable> {
		inline friend constexpr auto to_string(const Expr& expr) -> std::string {
			return expr.to_string();
		}

		[[nodiscard]] constexpr auto to_string() const -> std::string {
			return std::visit(
				overload {
					[](const BinOpExpr& expr) { return expr.to_string(); },
					[](const UnaryOpExpr& expr) { return expr.to_string(); },
					[](const Number& expr) { return expr.to_string(); },
					[](const Variable& expr) { return expr.to_string(); },
				},
				*this
			);
		}
	};

	namespace Errors {
		struct NotMatched {};

		struct RhsMiss {
			lex::Operator				 op;
			std::unique_ptr<Expr>		 lhs;

			inline friend constexpr auto to_string(const RhsMiss& err) -> std::string;
		};

		struct UnaryOperandMiss {
			lex::Operator				 op;

			inline friend constexpr auto to_string(const UnaryOperandMiss& err) -> std::string {
				return std::format("Loss operand for operator `{}`!", to_string(err.op));
			}
		};

		struct ParseError :
			public std::variant<  //
				NotMatched,		  //
				RhsMiss,		  //
				UnaryOperandMiss  //
				> {
			template<typename ErrorT>
			[[nodiscard]] auto is() const -> bool {
				return std::holds_alternative<ErrorT>(*this);
			}
		};

		template<typename ErrorT>
		inline static constexpr auto make_error(ErrorT&& err) {
			return tl::make_unexpected(ParseError { std::forward<ErrorT>(err) });
		}
	}  // namespace Errors

	using Errors::make_error;
	using Errors::ParseError;

	class Parser {};

	inline static auto parse(lex::TokenStream::View& ts, std::size_t min_bp = 0)
		-> tl::expected<Expr, ParseError>;
}  // namespace dcs213::p1::parse

namespace dcs213::p1::parse {
	inline static auto parse(lex::TokenStream::View& ts, std::size_t min_bp)
		-> tl::expected<Expr, ParseError> {
		if (const auto tok = ts.bump()) {
			const auto lhs = std::visit(
				overload {
					[&](const lex::Operator& tok) -> Expr {
						if (tok == lex::Operator::LParen) {
							auto lhs = parse(ts, 0);
							if (ts.expect({ lex::Operator::RParen }))
								return std::move(*lhs);
							else
								// return {};	// error
								throw std::runtime_error { "" };
						} else {
							return {};
						}
					},
					[](const lex::Number& tok) -> Expr { return { Number { tok.value } }; },
					[](const lex::Constant& tok) -> Expr { return { Number { val(tok) } }; },
					[](const lex::Variable& tok) -> Expr { return { Variable {} }; },
				},
				**tok
			);

			while (true) {
				const auto op = *std::get_if<lex::Operator>(&**ts.peek());	// TODO: if auto
				if (const auto sbp = bindpower[op].sbp) {}
			}
		}

		return {};
		return make_error(Errors::RhsMiss {});
	}
}  // namespace dcs213::p1::parse

namespace dcs213::p1::parse {
	inline static constexpr auto to_string(const BinOpExpr& expr) -> std::string {
		return std::format(
			"({} {} {})",
			to_string(*expr.lhs),
			lex::to_string(expr.op),
			to_string(*expr.rhs)
		);
	}

	inline constexpr auto UnaryOpExpr::to_string(const UnaryOpExpr& expr) -> std::string {
		return std::format("({} {})", lex::to_string(expr.op), expr.operand->to_string());
	}

	inline static constexpr auto Errors::to_string(const RhsMiss& err) -> std::string {
		return std::format(
			"Loss Right operand for operator `{}`, while the left operand is {}!",
			to_string(err.op),
			to_string(*err.lhs)
		);
	}
}  // namespace dcs213::p1::parse