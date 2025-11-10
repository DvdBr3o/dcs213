#pragma once

#include <cstddef>

namespace dcs213::project2 {
	template<typename SizeT = std::size_t>
	struct Pos {
		SizeT x;
		SizeT y;
	};

	template<typename DataT, typename SizeT = std::size_t>
	struct Triplet {
		using Pos = Pos<SizeT>;

		union {
			Pos pos;

			struct {
				SizeT x;
				SizeT y;
			};
		};

		DataT data;
	};

}  // namespace dcs213::project2