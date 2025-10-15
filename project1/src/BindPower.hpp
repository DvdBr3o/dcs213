#pragma once

#include "Map.hpp"
#include "Lexer.hpp"
#include "Utils.hpp"

#include <cstddef>
#include <array>
#include <utility>

namespace dcs213::p1 {
	struct BindPower {
		inline static constexpr auto unavailable = -1;

		struct Infix {
			int						  lbp;
			int						  rbp;

			inline constexpr explicit operator bool() const { return lbp > 0 && rbp > 0; }
		};

		//
		int	  pbp	= unavailable;
		Infix infix = {
			.lbp = unavailable,
			.rbp = unavailable,
		};
		int sbp = unavailable;
	};

	template<typename EnumT, std::size_t N>
	class AssoChain;
	template<typename EnumT>
	struct AssoPred;

	template<typename EnumT, std::size_t N, std::size_t... Idx>
	inline static constexpr auto
		make_asso_chain_impl(AssoPred<EnumT> (&&assos)[N], std::index_sequence<Idx...>)
			-> AssoChain<EnumT, N> {
		return { { std::move(assos)[Idx]... } };
	}

	template<typename EnumT, std::size_t N>
	[[nodiscard]] inline static constexpr auto make_asso_chain(AssoPred<EnumT> (&&assos)[N])
		-> AssoChain<EnumT, N> {
		return make_asso_chain_impl(std::move(assos), std::make_index_sequence<N>());
	}

	enum class Associativity {
		Left,
		Right,
		PrefixUnary,
		SuffixUnary,
	};

	template<typename EnumT>
	struct Asso {
		EnumT						 which;
		Associativity				 asso;

		inline static constexpr auto left(const EnumT& k) -> Asso {
			return { k, Associativity::Left };
		}

		inline static constexpr auto right(const EnumT& k) -> Asso {
			return { k, Associativity::Right };
		}

		inline static constexpr auto pre(const EnumT& k) -> Asso {
			return { k, Associativity::PrefixUnary };
		}

		inline static constexpr auto suf(const EnumT& k) -> Asso {
			return { k, Associativity::SuffixUnary };
		}

		inline constexpr auto operator>(Asso&& rhs) {
			return make_asso_chain({
				AssoPred<EnumT> { *this, AssoPred<EnumT>::Predecence::Prior },
				AssoPred<EnumT> {	  rhs, AssoPred<EnumT>::Predecence::Prior },
			});
		}

		inline constexpr auto operator>=(Asso&& rhs) {
			return make_asso_chain({
				AssoPred<EnumT> { *this, AssoPred<EnumT>::Predecence::Prior },
				AssoPred<EnumT> {	  rhs, AssoPred<EnumT>::Predecence::Equal },
			});
		}
	};

	template<typename EnumT>
	struct AssoPred {
		Asso<EnumT> asso;
		enum class Predecence {
			Prior,
			Equal,
		} pred;
	};

	template<typename EnumT, std::size_t N>
	class AssoChain : public std::array<AssoPred<EnumT>, N> {
	public:
		using Asso = Asso<EnumT>;
		using Pred = AssoPred<EnumT>::Predecence;

	public:
		template<std::size_t... Idx>
		constexpr AssoChain(AssoPred<EnumT> (&&assos)[N], std::index_sequence<Idx...>) :
			std::array<AssoPred<EnumT>, N> { { std::move(assos)[Idx]... } } {}

		constexpr AssoChain(AssoPred<EnumT> (&&assos)[N]) :
			AssoChain { std::move(assos), std::make_index_sequence<N>() } {}

		constexpr AssoChain(AssoChain<EnumT, N - 1>&& chain, AssoPred<EnumT>&& pred) :
			std::array<AssoPred<EnumT>, N>(concat_array(std::move(chain), std::move(pred))) {
			(*this)[N - 1] = pred;
		}

		template<std::size_t N1, std::size_t... Idx1, std::size_t... Idx2>
		constexpr AssoChain(AssoPred<EnumT> (&&assos1)[N1], AssoPred<EnumT> (&&assos2)[N - N1], std::index_sequence<Idx1...>, std::index_sequence<Idx2...>) :
			std::array<AssoPred<EnumT>, N> { std::move(assos1)[Idx1]...,
											 std::move(assos2)[Idx2]... } {}

		template<std::size_t N1>
		constexpr AssoChain(AssoPred<EnumT> (&&assos1)[N1], AssoPred<EnumT> (&&assos2)[N - N1]) :
			AssoChain { std::move(assos1),
						std::move(assos2),
						std::make_index_sequence<N1>(),
						std::make_index_sequence<N - N1>() } {}

	public:
		inline constexpr auto operator>(Asso&& rhs) && -> AssoChain<EnumT, N + 1> {
			return {
				std::move(*this),
				{ rhs, Pred::Prior }
			};
		}

		inline constexpr auto operator>=(Asso&& rhs) && -> AssoChain<EnumT, N + 1> {
			return {
				std::move(*this),
				{ rhs, Pred::Equal }
			};
		}

		template<std::size_t NN>
		inline constexpr auto operator>=(AssoChain<AssoPred<EnumT>, NN>&& rhs
		) && -> AssoChain<AssoPred<EnumT>, N + NN> {
			return make_asso_chain(concat_array(std::move(*this), std::move(rhs)));
		}

		inline friend constexpr auto build(const AssoChain& chain) -> EnumMap<EnumT, BindPower> {
			using EnumMap	 = EnumMap<EnumT, BindPower>;
			using AssoPred	 = AssoPred<EnumT>;
			using Predecence = AssoPred::Predecence;
			using std::pair;
			using std::size_t;

			BindPower  kv[EnumMap::count] = {};
			size_t	   counter			  = 0;
			size_t	   counter_mem		  = 0;
			Predecence curpred			  = Predecence::Prior;

			for (const auto& asso : std::ranges::reverse_view(chain)) {
				switch (curpred) {
					case Predecence::Prior:
						counter_mem = counter;
						switch (asso.asso.asso) {  // wtf asso.asso.asso?
							case Associativity::Left:
								kv[static_cast<std::size_t>(asso.asso.which)].infix.lbp = ++counter;
								kv[static_cast<std::size_t>(asso.asso.which)].infix.rbp = ++counter;
								break;
							case Associativity::Right:
								kv[static_cast<std::size_t>(asso.asso.which)].infix.rbp = ++counter;
								kv[static_cast<std::size_t>(asso.asso.which)].infix.lbp = ++counter;
								break;
							case Associativity::PrefixUnary:
								kv[static_cast<std::size_t>(asso.asso.which)].pbp = ++counter;
								break;
							case Associativity::SuffixUnary:
								kv[static_cast<std::size_t>(asso.asso.which)].sbp = ++counter;
								break;
						}
						break;
					case Predecence::Equal:
						const auto counter_mem_mem = counter_mem;
						switch (asso.asso.asso) {  // wtf asso.asso.asso?
							case Associativity::Left:
								kv[static_cast<std::size_t>(asso.asso.which)].infix.lbp =
									++counter_mem;
								kv[static_cast<std::size_t>(asso.asso.which)].infix.rbp =
									++counter_mem;
								break;
							case Associativity::Right:
								kv[static_cast<std::size_t>(asso.asso.which)].infix.rbp =
									++counter_mem;
								kv[static_cast<std::size_t>(asso.asso.which)].infix.lbp =
									++counter_mem;
								break;
							case Associativity::PrefixUnary:
								kv[static_cast<std::size_t>(asso.asso.which)].pbp = ++counter_mem;
								break;
							case Associativity::SuffixUnary:
								kv[static_cast<std::size_t>(asso.asso.which)].sbp = ++counter_mem;
								break;
						}
						counter_mem = counter_mem_mem;
						break;
				}
				curpred = asso.pred;
			}

			return { std::move(kv) };
		}
	};

	namespace asso {
		template<typename EnumT>
		inline static constexpr auto left(const EnumT& k) -> Asso<EnumT> {
			return { k, Associativity::Left };
		}

		template<typename EnumT>
		inline static constexpr auto right(const EnumT& k) -> Asso<EnumT> {
			return { k, Associativity::Right };
		}

		template<typename EnumT>
		inline static constexpr auto pre(const EnumT& k) -> Asso<EnumT> {
			return { k, Associativity::PrefixUnary };
		}

		template<typename EnumT>
		inline static constexpr auto suf(const EnumT& k) -> Asso<EnumT> {
			return { k, Associativity::SuffixUnary };
		}

	}  // namespace asso

	inline static constexpr auto operator_bindpower_factory() {
		using lex::Operator;
		using namespace asso;

		return build(
			pre(Operator::Plus)			 // unary +
			>= pre(Operator::Minus)		 // unary -
			> suf(Operator::Derivative)	 // '
			> right(Operator::Exponent)	 // ^
			> pre(Operator::Ln)			 // ln
			> left(Operator::Multiply)	 // *
			>= left(Operator::Devide)	 // /
			> left(Operator::Plus)		 // +
			>= left(Operator::Minus)	 // -
			> left(Operator::When)		 // $
		);
	}

	inline static constexpr auto bindpower = operator_bindpower_factory();

	inline static auto			 show_bindpower_table() -> void {
		  std::cout << std::format("op: pbp lbp rbp sbp\n");
		  for (const auto& [k, v] : bindpower)
			  std::cout << std::format(
				  "{:2}: {:3} {:3} {:3} {:3}\n",  //
				  to_string(k),
				  v.pbp,
				  v.infix.lbp,
				  v.infix.rbp,
				  v.sbp
			  );
	}

}  // namespace dcs213::p1
