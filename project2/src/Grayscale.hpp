#pragma once

#include <opencv2/opencv.hpp>

#include <filesystem>

namespace dcs213::project2 {
	inline static void
		grayscale(const std::string& image_path, const std::string& output_path, bool show = true) {
		const auto img = cv::imread(image_path);

		cv::Mat	   res;
		img.copyTo(res);

		res.forEach<cv::Vec3b>([](cv::Vec3b& pixel, const int* pos) {
			const unsigned char gray = .299 * pixel[0] + .587 * pixel[1] + .114 * pixel[2];
			pixel[0]				 = gray;
			pixel[1]				 = gray;
			pixel[2]				 = gray;
		});

		cv::imwrite(output_path, res);

		if (show) {
			cv::imshow(image_path, img);
			cv::imshow(output_path, res);
			cv::waitKey(0);
		}
	}
}  // namespace dcs213::project2