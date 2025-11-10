#pragma once

#include "Triplet.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <span>
#include <fstream>
#include <stdexcept>

namespace dcs213::project2 {
	struct alignas(std::size_t) RawTriplet {
		std::size_t total_size;
		std::size_t w;
		std::size_t h;
		// red channel triplets
		std::size_t rc_size;
		// green channel triplets
		std::size_t gc_size;
		// blue channel triplets
		std::size_t bc_size;
		// dynamic sizs data
		std::byte data[];

		using Ptr = std::unique_ptr<RawTriplet, decltype([](RawTriplet* p) {
										delete[] reinterpret_cast<char*>(p);
									})>;

		//
		inline static auto from(const cv::Mat& mat) -> Ptr {
			std::vector<cv::Mat> channels;
			cv::split(mat, channels);

			std::size_t sizes[3] = { 0 };
			std::size_t offsets[3];

			for (int c = 0; c < 3; ++c)
				for (int i = 0; i < channels[c].size[0]; ++i)
					for (int j = 0; j < channels[c].size[1]; ++j)
						if (channels[c].at<uchar>(i, j))
							++sizes[c];

			Ptr ptr { reinterpret_cast<RawTriplet*>(
				new char
					[offsetof(RawTriplet, data)
					 + (sizes[0] + sizes[1] + sizes[2]) * sizeof(Triplet<uchar>)]
			) };

			ptr->h			= mat.rows;
			ptr->w			= mat.cols;

			ptr->rc_size	= sizes[0];
			ptr->gc_size	= sizes[1];
			ptr->bc_size	= sizes[2];

			ptr->total_size = sizeof(RawTriplet) - sizeof(std::byte*)
							+ (ptr->rc_size + ptr->gc_size + ptr->bc_size) * sizeof(Triplet<uchar>);

			offsets[0] = 0;
			offsets[1] = sizes[0];
			offsets[2] = sizes[0] + sizes[1];

			for (int c = 0; c < 3; ++c) {
				int index = 0;
				for (std::size_t i = 0; i < channels[c].size[0]; ++i)
					for (std::size_t j = 0; j < channels[c].size[1]; ++j)
						if (const auto pixel = channels[c].at<uchar>(i, j))
							*(reinterpret_cast<Triplet<uchar>*>(ptr->data) + (offsets[c] + index++)
							) = { .x = i, .y = j, .data = pixel };
			}

			return ptr;
		}

		inline static auto from(const std::filesystem::path& path) -> Ptr {
			std::ifstream in { path, std::ios::binary };

			if (!in.is_open())
				throw std::runtime_error { std::format("Failed to open file {}!", path.string()) };

			in.seekg(0, std::ios::end);
			std::vector<uint8_t> content(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(reinterpret_cast<char*>(content.data()), content.size());

			in.close();

			RawTriplet* view = reinterpret_cast<RawTriplet*>(content.data());
			auto*		raw =
				reinterpret_cast<RawTriplet*>(new (std::nothrow) std::byte[view->total_size]);
			std::memcpy(raw, content.data(), sizeof(uint8_t) * content.size());
			return Ptr { raw };
		}

		[[nodiscard]] auto to_mat() const -> cv::Mat {
			cv::Mat		mat = cv::Mat::zeros(h, w, CV_8UC3);

			std::size_t sizes[3];
			std::size_t offsets[3];

			sizes[0]   = rc_size;
			sizes[1]   = gc_size;
			sizes[2]   = bc_size;
			offsets[0] = 0;
			offsets[1] = rc_size;
			offsets[2] = rc_size + gc_size;

			for (int c = 0; c < 3; ++c)
				for (int i = 0; i < sizes[c]; ++i) {
					const auto& triplet =
						*(reinterpret_cast<const Triplet<uchar>*>(data) + offsets[c] + i);
					mat.at<cv::Vec3b>(	//
						static_cast<int>(triplet.x),
						static_cast<int>(triplet.y)
					)[c] = triplet.data;
				}

			return mat;
		}

		void to_file(const std::filesystem::path& path) const {
			std::ofstream out { path, std::ios::binary };

			if (!out.is_open())
				throw std::runtime_error { std::format("Failed to open path {}!", path.string()) };

			out.write(reinterpret_cast<const char*>(this), total_size);

			out.close();
		}
	};

	inline static auto
		encode(const std::string& image_path, const std::string& output_path, bool show = false)
			-> void {
		const auto mat = cv::imread(image_path);
		RawTriplet::from(mat)->to_file(output_path);
		if (show) {
			cv::imshow(output_path, mat);
			cv::waitKey(0);
		}
	}

	inline static auto
		decode(const std::string& file_path, const std::string& output_path, bool show = false)
			-> void {
		const auto mat = RawTriplet::from(file_path)->to_mat();
		cv::imwrite(output_path, mat);
		if (show) {
			cv::imshow(output_path, mat);
			cv::waitKey(0);
		}
	}

}  // namespace dcs213::project2