#pragma once

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <string_view>

namespace dcs213::project2 {
	struct RescaleSize {
		std::size_t		   width;
		std::size_t		   height;

		inline static auto from(std::string_view s) -> RescaleSize {
			return {
				.width	= svtol(s.substr(0, s.find('x'))),
				.height = svtol(s.substr(s.find('x') + 1)),
			};
		}

	private:
		inline static auto svtol(std::string_view s) -> unsigned int {
			unsigned int res = 0;
			for (const auto c : s) res = res * 10 + c - '0';
			return res;
		}
	};

	inline static void rescale(
		const std::string& image_path,
		const std::string& output_path,
		const RescaleSize& size,
		bool			   show = false
	) {
		const auto img = cv::imread(image_path);
		if (((img.size[0] > size.height || img.size[1] > size.width)
			 && (img.size[0] % size.height || img.size[1] % size.width
				 || img.size[0] / size.height != img.size[1] / size.width))
			|| ((img.size[0] < size.height || img.size[1] < size.width
				 || img.size[0] / size.height != img.size[1] / size.width)
				&& (size.height % img.size[0] || size.width % img.size[1]))) {
			std::cerr << "Non-integer scaling is not supported yet!\n";
			return;
		}

		cv::Mat res = cv::Mat::zeros(size.height, size.width, CV_8UC3);

		if (img.size[0] > size.height) {  // zoom out
			const auto scale = img.size[0] / size.height;
			for (int i = 0; i < size.height; ++i)
				for (int j = 0; j < size.width; ++j) {
					cv::Vec3f pixel = 0;
					img(cv::Rect { static_cast<int>(j * scale),
								   static_cast<int>(i * scale),
								   static_cast<int>(scale),
								   static_cast<int>(scale) })
						.forEach<cv::Vec3b>([&](const cv::Vec3b& p, const int* pos) { pixel += p; }
						);
					pixel *= (1. / (scale * scale));
					res.at<cv::Vec3b>(i, j) = pixel;
				}
		} else {  // zoom in
			const auto scale = size.height / img.size[0];
			const auto sep	 = 1. / static_cast<double>(scale);
			for (int i = 0; i < img.size[0] - 1; ++i)
				for (int j = 0; j < img.size[1] - 1; ++j) {
					for (int x = 0; x < scale; ++x)
						for (int y = 0; y < scale; ++y)
							res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
								img.at<cv::Vec3b>(i, j) * (1 - sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i, j + 1) * (1 - sep * x) * (sep * y) +  //
								img.at<cv::Vec3b>(i + 1, j) * (sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i + 1, j + 1) * (sep * x) * (sep * y)	   //
								;
				}
			{
				const int i = img.size[0] - 1;
				for (int j = 0; j < img.size[1] - 1; ++j) {
					for (int x = 0; x < scale; ++x)
						for (int y = 0; y < scale; ++y)
							res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
								img.at<cv::Vec3b>(i - 1, j) * (1 - sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i - 1, j + 1) * (1 - sep * x) * (sep * y) +  //
								img.at<cv::Vec3b>(i, j) * (sep * x) * (1 - sep * y) +		   //
								img.at<cv::Vec3b>(i, j + 1) * (sep * x) * (sep * y)			   //
								;
				}
			}
			{
				const int j = img.size[1] - 1;
				for (int i = 0; i < img.size[1] - 1; ++i) {
					for (int x = 0; x < scale; ++x)
						for (int y = 0; y < scale; ++y)
							res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
								img.at<cv::Vec3b>(i, j - 1) * (1 - sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i, j) * (1 - sep * x) * (sep * y) +		   //
								img.at<cv::Vec3b>(i + 1, j - 1) * (sep * x) * (1 - sep * y) +  //
								img.at<cv::Vec3b>(i + 1, j) * (sep * x) * (sep * y)			   //
								;
				}
			}
			{
				const int i = img.size[0] - 1;
				const int j = img.size[1] - 1;
				for (int x = 0; x < scale; ++x)
					for (int y = 0; y < scale; ++y)
						res.at<cv::Vec3b>(i * scale + x, j * scale + y) =
							img.at<cv::Vec3b>(i - 1, j - 1) * (1 - sep * x) * (1 - sep * y) +  //
							img.at<cv::Vec3b>(i - 1, j) * (1 - sep * x) * (sep * y) +		   //
							img.at<cv::Vec3b>(i, j - 1) * (sep * x) * (1 - sep * y) +		   //
							img.at<cv::Vec3b>(i, j) * (sep * x) * (sep * y)					   //
							;
			}
		}

		cv::imwrite(output_path, res);

		if (show) {
			cv::imshow(image_path, img);
			cv::imshow(output_path, res);
			cv::waitKey(0);
		}
	}
}  // namespace dcs213::project2