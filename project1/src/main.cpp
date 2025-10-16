#include "View.hpp"

using namespace dcs213::p1;

//
#if defined DCS213_P1_PLAT_WINDOWS
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#else
int main(int argc, char** argv) {
#endif
	MainView view {
		{
			.debug	= true,
			.width	= 400,
			.height = 560,
			.title	= "DCS213 P1 YatCalculator",
			.ui		= ui,
		 }
	};
	view.run();
}