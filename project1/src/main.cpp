#include "BindPower.hpp"
#include "Lexer.hpp"
#include "View.hpp"

using namespace dcs213::p1;

//
int main(int argc, char** argv) {
	// if (auto ts = lex::lex("3.1415+222 * pi - e * (x ^ 2 ^ 3 + x * e)")) {
	// 	std::cout << ts->size() << '\n';
	// 	for (const auto& tok : *ts) std::cout << tok.to_string() << '\n';
	// }

	std::cout << std::format("op: pbp lbp rbp sbp\n");
	for (const auto& [k, v] : bindpower)
		std::cout << std::format(
			"{:2}: {:3} {:3} {:3} {:3}\n",	//
			to_string(k),
			v.pbp,
			v.lbp,
			v.rbp,
			v.sbp
		);

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
