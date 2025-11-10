#include "CommandLine.hpp"

#include <opencv2/opencv.hpp>
#include <clipp.h>

using namespace dcs213::project2;

int main(int argc, char** argv) {
	try {
		CommandLine(argc, argv);
	} catch (const std::exception& e) { std::cout << e.what() << '\n'; }
}