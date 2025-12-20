#define NOMINMAX
#include "CommandLine.hpp"
#include "Graph.hpp"

#include <Windows.h>

using namespace dcs213::project4;

int main(int argc, char** argv) {
	SetConsoleOutputCP(65001);
	try {
		CommandLine(argc, argv);
	} catch (const std::exception& e) { std::cout << e.what() << '\n'; }
}