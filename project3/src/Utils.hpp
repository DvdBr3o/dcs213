#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <span>

namespace dcs213::project3 {
	inline static auto bin_from(const std::filesystem::path& path) {
		std::ifstream in { path, std::ios::binary };

		if (!in.is_open())
			throw std::runtime_error { std::format("Failed to open file {}!", path.string()) };

		in.seekg(0, std::ios::end);
		std::vector<char> data(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(reinterpret_cast<char*>(data.data()), data.size());

		in.close();

		return data;
	}

	inline static auto bin_to(const std::filesystem::path& path, std::span<const char> data) {
		std::ofstream out { path, std::ios::binary };
		if (!out.is_open())
			throw std::runtime_error { std::format("Failed to open path {}!", path.string()) };

		out.write(reinterpret_cast<const char*>(data.data()), data.size());

		out.close();
	}
}  // namespace dcs213::project3