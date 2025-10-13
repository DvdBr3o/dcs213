#pragma once

#include <string_view>

namespace dcs213::p1 {
	class Cursor : public std::string_view {
	public:
		inline static constexpr char Eof = 0;

	public:
		using std::string_view::string_view;

		constexpr Cursor(std::string_view view) : std::string_view(view) {}

	public:
		constexpr auto bump() -> char {
			if (_index == size())
				return Eof;
			return (*this)[_index++];
		}

		constexpr auto first(std::size_t n = 0) {
			if (_index + n < size())
				return (*this)[_index + n];
			else
				return Eof;
		}

		constexpr auto rest(int offset = 0) -> std::string_view { return substr(_index + offset); }

	private:
		std::size_t _index = 0;
	};
}  // namespace dcs213::p1