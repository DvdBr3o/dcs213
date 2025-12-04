#include "CommandLine.hpp"

#include <iostream>
#include <format>

using namespace dcs213::project3;

const char* args[] = {
	R"(C:\Users\DvdBr3o\devenv\dcs213\build\windows\x64\debug\dcs213.project3.exe)",
	"encode",
	"public/standard_test_images/house.tif",
	"-o",
	"house.huf",
};

int main(int argc, char** argv) {
	try {
		// CommandLine(sizeof(args) / sizeof(args[0]), (char**)args);
		CommandLine(argc, argv);
	} catch (const std::exception& e) {
		std::cerr << std::format("Uncaught exception: {}\n", e.what());
	}
}