#pragma once

#include <simdjson.h>
#include <utility>

namespace dcs213::p1 {
	namespace details {
		template<size_t I, typename... Args>
		struct get_type_n {};

		template<size_t I, typename T, typename... Args>
		struct get_type_n<I, T, Args...> {
			using type = get_type_n<I - 1, Args...>::type;
		};

		template<typename T, typename... Args>
		struct get_type_n<0, T, Args...> {
			using type = T;
		};

		template<size_t I>
		struct get_type_n<I> {
			static_assert(false, "Index out of bounds");
		};
	}  // namespace details

	using details::get_type_n;

	/**
	 * @brief Get the n-th type of a type parameter pack
	 *
	 * @tparam I index
	 * @tparam Args the type parameter pack
	 */
	template<size_t I, typename... Args>
	using get_type_n_t = details::get_type_n<I, Args...>::type;

	/**
	 * @brief boilertemplate for polymorphism pattern matching of `std::visit` & `std::variant`.
	 *
	 * If you know it, you know it.
	 *
	 * @tparam Ts
	 */
	template<typename... Ts>
	struct overload : Ts... {
		using Ts::operator()...;
	};

	template<typename T, std::size_t N, std::size_t... Idx>
	inline static constexpr auto
		concat_array_impl(std::array<T, N>&& arr, T&& item, std::index_sequence<Idx...>)
			-> std::array<T, N + 1> {
		return {
			{ std::move(arr)[Idx]..., std::forward<T>(item) }
		};
	}

	template<typename T, std::size_t N1, std::size_t N2, std::size_t... Idx1, std::size_t... Idx2>
	inline static constexpr auto
		concat_array_impl(std::array<T, N1>&& arr1, std::array<T, N2>&& arr2, std::index_sequence<Idx1...>, std::index_sequence<Idx2...>)
			-> std::array<T, N1 + N2> {
		return {
			{ std::move(arr1)[Idx1]..., std::move(arr2)[Idx2]... }
		};
	}

	/**
	 * @brief constexpr-ly append an element onto an array.
	 *
	 * @tparam T
	 * @tparam N
	 * @param arr
	 * @param item
	 * @return std::array<T, N + 1>
	 */
	template<typename T, std::size_t N>
	inline static constexpr auto concat_array(std::array<T, N>&& arr, T&& item)
		-> std::array<T, N + 1> {
		return concat_array_impl(
			std::move(arr),
			std::forward<T>(item),
			std::make_index_sequence<N>()
		);
	}

	/**
	 * @brief constexpr-ly concat an array with an array.
	 *
	 * @tparam T
	 * @tparam N1
	 * @tparam N2
	 * @param arr1
	 * @param arr2
	 * @return std::array<T, N1 + N2>
	 */
	template<typename T, std::size_t N1, std::size_t N2>
	inline static constexpr auto concat_array(std::array<T, N1>&& arr1, std::array<T, N2>&& arr2)
		-> std::array<T, N1 + N2> {
		return concat_array_impl(
			std::move(arr1),
			std::move(arr2),
			std::make_index_sequence<N1>(),
			std::make_index_sequence<N2>()
		);
	}
}  // namespace dcs213::p1

namespace dcs213::p1::json {
	inline static simdjson::ondemand::parser parser {};

	namespace details {
		template<size_t I, typename... Args>
		auto init_args(std::tuple<Args...>& args, simdjson::padded_string json) -> void {
			if constexpr (I >= sizeof...(Args))
				return;
			auto doc		  = parser.iterate(json);
			std::get<I>(args) = doc.at(I).get<get_type_n_t<I, Args...>>().take_value();
			init_args<I + 1>(args, doc);
		}

		template<typename... Args>
		auto init_args(std::tuple<Args...>& args, simdjson::padded_string json) -> void {
			init_args<0>(args, json);
		}

		template<typename... Args>
		auto init_args(std::tuple<Args...>& args, const std::string& json) -> void {
			init_args<0>(args, json);
		}

		template<>
		auto init_args(std::tuple<>& args, const std::string& json) -> void {
			// No-op for empty tuple
		}
	}  // namespace details

	using details::init_args;
}  // namespace dcs213::p1::json