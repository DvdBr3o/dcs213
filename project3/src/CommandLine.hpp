#pragma once

#include "Codec.hpp"

#include <clipp.h>

#include <iostream>

namespace dcs213::project3 {
	class CommandLine {
	public:
		enum class Mode {
			Help,
			Test,
			Encode,
			Decode,
		};

	public:
		CommandLine(int argc, char** argv) {
			using namespace clipp;

			// for (int i = 0; i < argc; ++i) std::cout << argv[i] << '\n';

			const auto output = (required("--output", "-o"), value("output").set(_output_path));

			const auto help =
				command("help", "--help", "-h").set(_mode, Mode::Help) % "Show help message";
			const auto test =
				(command("test").set(_mode, Mode::Test), value("img_path").set(_img_path))
				% "Test huffman codec with specific image";
			const auto encode = (command("encode").set(_mode, Mode::Encode),
								 value("img_path").set(_img_path),
								 output)
							  % "Encode images";
			const auto decode = (command("decode").set(_mode, Mode::Decode),
								 value("img_path").set(_img_path),
								 output)
							  % "Decode images";

			const auto cli = (help | test | encode | decode);

			if (!parse(argc, argv, cli))
				std::cout << make_man_page(cli);

			switch (_mode) {
				case Mode::Help: std::cout << make_man_page(cli); break;
				case Mode::Test:
					decode_halffman(encode_halffman(_img_path, "test.huf"), "test.png");
					break;
				case Mode::Encode: encode_halffman(_img_path, _output_path); break;
				case Mode::Decode: decode_halffman(_img_path, _output_path); break;
			}
		}

	private:
		Mode		_mode;
		bool		_profile;
		std::string _img_path;
		std::string _output_path;
	};
}  // namespace dcs213::project3