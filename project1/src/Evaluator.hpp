#pragma once

#include "Lexer.hpp"
#include "Parser.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace dcs213::p1::evaluate {
	struct TermList;

	/**
	 * @brief Represent a algebra term.
	 *
	 * In term of:
	 * $$
	 * \mathrm{coef} x^{\mathrm{expo}}
	 * $$
	 *
	 */
	struct Term {
		using List	= TermList;

		double coef = 1.;  // coefficient
		double expo = 1.;  // exponent

		//
		[[nodiscard]] auto to_string() const -> std::string {
			if (expo == 0.)
				return std::format("{}", coef);
			else if (coef == 1. && expo == 1.)
				return std::format("x");
			else if (coef == -1. && expo == 1.)
				return std::format("-x");
			else if (coef == 1. && expo != 1.)
				return std::format("x^{}", expo);
			else if (coef == -1. && expo != 1.)
				return std::format("-x^{}", expo);
			else if (coef != 1. && expo == 1.)
				return std::format("{}*x", coef);
			else
				return std::format("{}*x^{}", coef, expo);
		}

		[[nodiscard]] auto derivative() const -> std::optional<Term> {
			if (expo == 0)
				return Term {
					.coef = 0,
					.expo = 0,
				};
			else if (expo == -1)
				return std::nullopt;  // Currently we don not support (1/x)'=lnx, since it obey the
									  // termlist form.
			else
				return Term {
					.coef = coef * expo,
					.expo = expo - 1.,
				};
		}
	};

	/**
	 * @brief Represents a list of term (Polynomial 多项式)
	 *
	 */
	class TermList : public std::vector<Term> {
	public:
		using vector::iterator;
		using vector::vector;

	public:
		inline friend auto operator+(const TermList& lhs, const TermList& rhs) -> TermList {
			TermList						   res;

			std::unordered_map<double, double> terms;

			for (const auto& [c, e] : lhs)
				if (terms.find(e) != terms.end())
					terms[e] += c;
				else
					terms[e] = c;

			for (const auto& [c, e] : rhs)
				if (terms.find(e) != terms.end())
					terms[e] += c;
				else
					terms[e] = c;

			res.reserve(terms.size());

			for (const auto& [e, c] : terms) res.emplace_back(c, e);

			res._sort_self();

			return res;	 // nrvo
		}

		inline friend auto operator-(const TermList& lhs, const TermList& rhs) -> TermList {
			TermList						   res;

			std::unordered_map<double, double> terms;

			for (const auto& [c, e] : lhs)
				if (terms.find(e) != terms.end())
					terms[e] += c;
				else
					terms[e] = c;

			for (const auto& [c, e] : rhs)
				if (terms.find(e) != terms.end())
					terms[e] -= c;
				else
					terms[e] = -c;

			res.reserve(terms.size());

			for (const auto& [e, c] : terms) res.emplace_back(c, e);

			res._sort_self();

			return res;	 // nrvo
		}

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

		[[nodiscard]] auto derivative() const -> std::optional<TermList> {
			TermList terms;
			terms.reserve(size());

			for (const auto term : *this)
				if (const auto d = term.derivative()) {
					if (d->coef != 0.)
						terms.emplace_back(*d);
				} else
					return std::nullopt;

			return terms;  // nrvo
		}

		[[nodiscard]] auto eval(double x) const -> double {
			double res = 0.;

			for (const auto [c, e] : *this) res += c * std::pow(x, e);

			return res;
		}

		[[nodiscard]] auto to_string() const -> std::string {
			std::string s;

			s += (*this)[0].to_string();

			for (int i = 1; i < size(); ++i)
				s += std::format("{}{}", (*this)[i].coef > 0 ? "+" : "", (*this)[i].to_string());

			return s;  // nrvo
		}

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

	template<std::invocable<const parse::Expr&>... Ts>
	class handle_eval : std::tuple<Ts...> {
	public:
		explicit constexpr handle_eval(Ts&&... ts) : std::tuple<Ts...>(std::forward<Ts>(ts)...) {}

	public:
		template<std::size_t I0, std::size_t... Is>
		[[nodiscard]] constexpr auto apply(const parse::Expr& expr, std::index_sequence<I0, Is...>)
			const -> std::optional<parse::Expr> {
			if (auto&& res = std::get<I0>(*this)(expr))
				return std::move(res);
			else
				return apply(expr, std::index_sequence<Is...>());
		}

		[[nodiscard]] constexpr auto apply(const parse::Expr& expr, std::index_sequence<>) const
			-> std::optional<parse::Expr> {
			return std::nullopt;
		}

		constexpr auto operator()(const parse::Expr& expr) const -> std::optional<parse::Expr> {
			return apply(expr, std::make_index_sequence<sizeof...(Ts)>());
		}
	};

	inline static auto eval_con(const parse::Expr& expr) -> std::optional<double>;
	inline static auto eval_var(const parse::Expr& expr) -> parse::Expr;

	inline static auto eval_nocoef_term(const parse::Expr& expr) -> std::optional<double> {
		// std::cout << std::format("parsing nocoef term: {}\n", expr.to_string());
		if (const auto binop = expr.get_if<parse::BinOpExpr>()) {
			if (binop->op == lex::Operator::Exponent) {
				if (binop->lhs->is<parse::Variable>()) {
					if (auto expo = eval_con(*binop->rhs))
						return *expo;
				}
				if (binop->rhs->is<parse::Variable>()) {
					if (auto expo = eval_con(*binop->rhs))
						return *expo;
				}
			}
		}
		if (const auto var = expr.get_if<parse::Variable>())
			return 1.;

		return std::nullopt;
	}

	inline static auto eval_term(const parse::Expr& expr) -> std::optional<Term> {
		// std::cout << std::format("parsing term: {}\n", expr.to_string());
		if (const auto binop = expr.get_if<parse::BinOpExpr>())
			if (binop->op == lex::Operator::Multiply) {	 // c * x ^ e
				if (const auto expo = eval_nocoef_term(*binop->lhs))
					if (auto coef = eval_con(*binop->rhs))
						return Term {
							.coef = *coef,
							.expo = *expo,
						};

				if (const auto expo = eval_nocoef_term(*binop->rhs))
					if (auto coef = eval_con(*binop->lhs))
						return Term {
							.coef = *coef,
							.expo = *expo,
						};
			}

		if (auto expo = eval_nocoef_term(expr))					   // x ^ e
			return Term { .coef = 1., .expo = *expo };
		else if (const auto var = expr.get_if<parse::Variable>())  // x
			return Term { .coef = 1., .expo = 1. };
		else if (const auto num = expr.get_if<parse::Number>())
			return Term { .coef = num->val, .expo = 0. };

		return std::nullopt;
	}

	inline static auto eval_termlist(const parse::Expr& expr) -> std::optional<TermList> {
		// std::cout << std::format("parsing term list: {}\n", expr.to_string());
		if (auto term = eval_term(expr))
			return TermList { *term };
		else if (const auto binop = expr.get_if<parse::BinOpExpr>())
			if (const auto lhs = eval_termlist(*binop->lhs))
				if (const auto rhs = eval_termlist(*binop->rhs))
					switch (binop->op) {
						case lex::Operator::Plus: return *lhs + *rhs;
						case lex::Operator::Minus: return *lhs - *rhs;
						default: return std::nullopt;
					}

		return std::nullopt;
	}

	inline static auto eval_termlist_calc(const parse::Expr& expr) -> std::optional<TermList> {
		if (const auto binop = expr.get_if<parse::BinOpExpr>()) {
			if (const auto lhs = eval_termlist(*binop->lhs)) {
				if (const auto rhs = eval_termlist(*binop->rhs)) {
					switch (binop->op) {
						case lex::Operator::Plus: return *lhs + *rhs;
						case lex::Operator::Minus: return *lhs - *rhs;
						case lex::Operator::Multiply: return *lhs * *rhs;
						default: break;
					}
				}
				if (const auto rhs = eval_con(*binop->rhs)) {
					if (binop->op == lex::Operator::When)
						return TermList {
							Term { .coef = lhs->eval(*rhs), .expo = 0. }
						};
				}
			}
		}

		if (const auto uop = expr.get_if<parse::UnaryOpExpr>())
			if (const auto oper = eval_termlist(*uop->operand))
				switch (uop->op) {
					case lex::Operator::Derivative:
						if (auto d = oper->derivative())
							return std::move(d);
					default: return std::nullopt;
				}

		return std::nullopt;
	}

	inline static auto eval_var(const parse::Expr& expr) -> parse::Expr {
		// static constexpr auto handle = handle_eval {
		// 	&eval_termlist_add,
		// 	&eval_termlist_mul,
		// };

		// handle(expr);
	}

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

	inline static auto eval(const parse::Expr& expr) -> std::optional<std::string> {
		if (const auto res = eval_con(expr))
			return std::format("{}", *res);
		else if (const auto terms = eval_termlist_calc(expr))
			return std::format("{}", terms->to_string());

		return std::nullopt;
	}
}  // namespace dcs213::p1::evaluate
