#pragma once

#include <magic_enum.hpp>

#include <cstddef>
#include <utility>

namespace dcs213::p1 {
	template<typename EnumT, typename V>
	class EnumMap {
	public:
		inline static constexpr auto count = magic_enum::enum_count<EnumT>();

		using iterator					   = class Iterator {
		public:
			constexpr Iterator(const EnumMap& self, std::size_t index) :
				_self(self), _index(index) {}

		public:
			constexpr auto& operator++() {
				++_index;
				return *this;
			}

			constexpr auto operator++(int) -> Iterator {
				auto origin = *this;
				++(*this);
				return origin;	// nrvo
			}

			constexpr auto operator*() -> std::pair<EnumT, const V&> {
				return { *magic_enum::enum_cast<EnumT>(_index), _self._map[_index] };
			}

			inline friend constexpr auto operator==(const Iterator& lhs, const Iterator& rhs)
				-> bool {
				return &lhs._self == &rhs._self && lhs._index == rhs._index;
			}

		private:
			const EnumMap& _self;
			std::size_t	   _index;
		};

	public:
		template<std::size_t N>
		constexpr EnumMap(std::pair<EnumT, V> (&&kv)[N]) {
			for (auto&& kv : kv) _map[static_cast<std::size_t>(kv.first)] = kv.second;
		}

		constexpr EnumMap(V (&&kv)[count]) { std::copy(kv, kv + count, _map); }

	public:
		constexpr auto operator[](EnumT k) const -> const V& {
			return _map[static_cast<std::size_t>(k)];
		}

		constexpr auto operator[](EnumT k) -> V& { return _map[static_cast<std::size_t>(k)]; }

		constexpr auto begin() const -> Iterator { return { *this, 0 }; }

		constexpr auto end() const -> Iterator { return { *this, count }; }

	private:
		V _map[count] { 0 };
	};

	template<typename EnumT, typename V, std::size_t N>
	inline static constexpr auto make_enum_map(std::pair<EnumT, V> (&&kv)[N]) -> EnumMap<EnumT, V> {
		return EnumMap { kv };
	}

}  // namespace dcs213::p1