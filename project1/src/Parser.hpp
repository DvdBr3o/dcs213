#pragma once

#include "BindPower.hpp"
#include "Lexer.hpp"

#include <tl/expected.hpp>

#include <memory>
#include <variant>
#include <exception>

namespace dcs213::p1::parse {
	struct Expr;

	/**
	 * @brief Represents a sub-expression of binary-operand operator.
	 *
	 * e.g.
	 *      +
	 *    /   \
	 *  1      2
	 *
	 */
	struct BinOpExpr {
		lex::Operator		  op;
		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		inline friend auto	  to_string(const BinOpExpr& expr) -> std::string {
			   return expr.to_string();
		}

		[[nodiscard]] auto to_string() const -> std::string;
	};

	/**
	 * @brief Represents a sub-expression of unary-operand operator.
	 *
	 * e.g.
	 *    ln
	 *    |
	 *    42
	 *
	 */
	struct UnaryOpExpr {
		lex::Operator		  op;
		std::unique_ptr<Expr> operand;

		[[nodiscard]] auto	  to_string() const -> std::string;

		inline friend auto	  to_string(const UnaryOpExpr& expr) -> std::string {
			   return expr.to_string();
		}
	};

	/**
	 * @brief Represents a known number literal atomic expression.
	 *
	 */
	struct Number {
		double			   val;

		inline static auto to_string(const Number& num) -> std::string {
			return std::format("{}", num.val);
		}

		[[nodiscard]] auto to_string() const -> std::string { return to_string(*this); }
	};

	/**
	 * @brief Represents a variable atomic expression.
	 *
	 */
	struct Variable {
		inline static constexpr auto to_string(const Variable& expr) -> std::string {
			// return std::format("{} {}", lex::to_string(expr.op),);
			return "x";
		}

		[[nodiscard]] constexpr auto to_string() const -> std::string { return to_string(*this); }
	};

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

	namespace Errors {
		/**
		 * @brief Just a placeholder error. Ignore it.
		 *
		 */
		struct NotMatched {
			[[nodiscard]] inline static auto to_string() -> std::string { return "Not Matched!"; }
		};

		/**
		 * @brief An error that right hand side of a binary-operand operator is missed.
		 *
		 * e.g. in `(1+) * 2` the rhs is missed in `1+`.
		 */
		struct RhsMiss {
			lex::Operator		  op;
			std::unique_ptr<Expr> lhs;

			[[nodiscard]] auto	  to_string() const -> std::string;
		};

		/**
		 * @brief An error that the operand of a unary-operand operator is missed.
		 *
		 */
		struct UnaryOperandMiss {
			lex::Operator	   op;

			[[nodiscard]] auto to_string() const -> std::string {
				return std::format("Loss operand for operator `{}`!", lex::to_string(op));
			}
		};

		/**
		 * @brief An error that the right parenthesis corresponding to its left one is missed.
		 *
		 */
		struct RParenMiss {
			[[nodiscard]] inline static auto to_string() -> std::string {
				return "Right Parenthesis Missed!";
			}
		};

		inline static auto to_string(const auto& err) -> std::string {
			return err.to_string();
		}

		struct ParseError :
			public std::variant<   //
				NotMatched,		   //
				RhsMiss,		   //
				UnaryOperandMiss,  //
				RParenMiss		   //
				> {
			template<typename ErrorT>
			[[nodiscard]] auto is() const -> bool {
				return std::holds_alternative<ErrorT>(*this);
			}

			/**
			 * @brief Forwarding `.to_string()`.
			 *
			 * @return std::string
			 */
			[[nodiscard]] auto to_string() const -> std::string {
				return std::visit([](const auto& err) { return err.to_string(); }, *this);
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

	/**
	 * @brief Parse a token stream into an AST.
	 *
	 * @param ts token stream
	 * @param min_bp minimum binding power
	 * @return tl::expected<Expr, ParseError>
	 */
	inline static auto parse(lex::TokenStream::View& ts, std::size_t min_bp = 0)
		-> tl::expected<Expr, ParseError>;

	/**
	 * @brief Parse a token stream into an AST.
	 *
	 * @param ts
	 * @param min_bp
	 * @return tl::expected<Expr, ParseError>
	 */
	inline static auto parse(lex::TokenStream::View&& ts, std::size_t min_bp = 0)
		-> tl::expected<Expr, ParseError> {
		return parse(static_cast<lex::TokenStream::View&>(ts));
	}

	/**
	 * @brief Parse a token stream into an AST.
	 *
	 * @param ts
	 * @param min_bp
	 * @return tl::expected<Expr, ParseError>
	 */
	inline static auto parse(const lex::TokenStream& ts, std::size_t min_bp = 0)
		-> tl::expected<Expr, ParseError> {
		return parse(ts.view());
	}

}  // namespace dcs213::p1::parse

namespace dcs213::p1::parse {
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
					else
						return make_error(Errors::UnaryOperandMiss { .op = *op });
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
}  // namespace dcs213::p1::parse

namespace dcs213::p1::parse {
	inline auto BinOpExpr::to_string() const -> std::string {
		return std::format("({} {} {})", lhs->to_string(), lex::to_string(op), rhs->to_string());
	}

	inline auto UnaryOpExpr::to_string() const -> std::string {
		return std::format("({} {})", lex::to_string(op), operand->to_string());
	}

	inline auto Errors::RhsMiss::to_string() const -> std::string {
		return std::format(
			"Loss Right operand for operator `{}`, while the left operand is {}!",
			lex::to_string(op),
			lhs->to_string()
		);
	}

}  // namespace dcs213::p1::parse
