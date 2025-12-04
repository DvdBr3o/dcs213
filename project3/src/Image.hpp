#pragma once

#include <stb/stb_image.h>
#include <opencv2/opencv.hpp>

#include <filesystem>
#include <stdexcept>

namespace dcs213::project3 {
	class Image {
	public:
		struct Pixel {
			union {
				struct {
					std::uint8_t r;
					std::uint8_t g;
					std::uint8_t b;
					std::uint8_t a;
				};

				std::uint32_t rgba;
			};
		};

		struct Size {
			int width;
			int height;
		};

		using iterator = struct Iterator {
		public:
			Iterator(Pixel* data, std::size_t index) : _data(data), _index(index) {}

		public:
			auto&		operator*() { return _data[_index]; }

			const auto& operator*() const { return _data[_index]; }

			auto*		operator->() { return &_data[_index]; }

			const auto* operator->() const { return &_data[_index]; }

			auto&		operator++() {
				  ++_index;
				  return *this;
			}

			auto operator++(int) {
				auto origin = *this;
				++_index;
				return origin;
			}

			inline friend constexpr auto operator==(const Iterator& lhs, const Iterator& rhs)
				-> bool {
				return lhs._data == rhs._data && lhs._index == rhs._index;
			}

		private:
			Pixel*		_data;
			std::size_t _index;
		};

	public:
		Image(const std::filesystem::path& path) {
			if (!std::filesystem::exists(path))
				throw std::runtime_error {
					std::format("Path \"{}\" does not exist!", path.string())
				};

			_data = stbi_load(	//
				path.string().data(),
				&_width,
				&_height,
				&_channels,
				4
			);
			if (!_data)
				throw std::runtime_error {
					std::format("Failed to load image at {}", path.string())
				};
		}

		[[nodiscard]] auto channel_count() const { return _channels; }

		[[nodiscard]] auto size() const -> Size { return { _width, _height }; }

		[[nodiscard]] auto at(int x, int y) const -> Pixel {
			return reinterpret_cast<Pixel*>(_data)[y * _height + x];
		}

		auto			   begin() -> Iterator { return { reinterpret_cast<Pixel*>(_data), 0 }; }

		[[nodiscard]] auto begin() const -> Iterator {
			return { reinterpret_cast<Pixel*>(_data), 0 };
		}

		auto end() -> Iterator {
			return { reinterpret_cast<Pixel*>(_data), static_cast<size_t>(_width * _height) };
		}

		[[nodiscard]] auto end() const -> Iterator {
			return { reinterpret_cast<Pixel*>(_data), static_cast<size_t>(_width * _height) };
		}

	private:
		void* _data;
		int	  _width;
		int	  _height;
		int	  _channels;
	};
}  // namespace dcs213::project3