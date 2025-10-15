#include "BindPower.hpp"
#include "Evaluator.hpp"
#include "Lexer.hpp"
#include "View.hpp"
#include "Parser.hpp"

using namespace dcs213::p1;

//
int main(int argc, char** argv) {
	show_bindpower_table();
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
