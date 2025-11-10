#pragma once

#include "Grayscale.hpp"
#include "Codec.hpp"
#include "Rescale.hpp"
#include "opencv2/highgui.hpp"

#include <clipp.h>

#include <iostream>

namespace dcs213::project2 {
	class CommandLine {
	public:
		enum class Mode {
			Help,
			Grayscale,
			Rescale,
			Encode,
			Decode,
			Show,
		};

	public:
		CommandLine(int argc, char** argv) {
			using namespace clipp;

			auto image	   = value("image").set(_image_path);
			auto output	   = (required("--output", "-o"), value("output").set(_output_path));

			auto grayscale = (command("grayscale").set(_mode, Mode::Grayscale), image, output);

			auto rescale =
				(command("rescale").set(_mode, Mode::Rescale),
				 image,
				 output,
				 required("--size", "-s"),
				 value("size").set(_rescale_size));

			auto encode = (command("encode").set(_mode, Mode::Encode), image, output);

			auto decode = (command("decode").set(_mode, Mode::Decode), image, output);

			auto help	= command("help", "--help", "-h").set(_mode, Mode::Help);

			auto cli =
				((grayscale | rescale | encode | decode | help,
				  option("--show").set(_show, true) | option("--hide").set(_show, false))
				 | (command("show").set(_mode, Mode::Show), image));

			if (!parse(argc, argv, cli))
				std::cout << make_man_page(cli);

			switch (_mode) {
				case Mode::Grayscale: project2::grayscale(_image_path, _output_path, _show); break;
				case Mode::Encode: project2::encode(_image_path, _output_path, _show); break;
				case Mode::Decode: project2::decode(_image_path, _output_path, _show); break;
				case Mode::Rescale:
					project2::rescale(
						_image_path,
						_output_path,
						RescaleSize::from(_rescale_size),
						_show
					);
					break;
				case Mode::Help: std::cout << make_man_page(cli) << '\n'; break;
				case Mode::Show:
					cv::imshow(_image_path, cv::imread(_image_path));
					cv::waitKey(0);
					break;
			}
		}

	private:
		Mode		_mode;
		std::string _image_path;
		std::string _output_path;
		std::string _rescale_size;
		bool		_show;
	};
}  // namespace dcs213::project2