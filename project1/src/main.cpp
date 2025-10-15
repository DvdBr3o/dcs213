#include "BindPower.hpp"
#include "Evaluator.hpp"
#include "Lexer.hpp"
#include "View.hpp"
#include "Parser.hpp"

using namespace dcs213::p1;

//
int main(int argc, char** argv) {
	// std::string in;
	// std::cin >> in;

	// const auto ts = *lex::lex(in);

	// std::cout << to_string(ts);

	// const auto ast = parse::parse(ts);

	// if (ast) {
	// 	std::cout << "AST:\n";
	// 	std::cout << ast->to_string() << '\n';
	// } else {
	// 	std::cout << "Failed to parse!\n";
	// 	std::cout << ast.error().to_string() << '\n';
	// 	return 0;
	// }

	// if (const auto term = evaluate::eval_termlist_calc(*ast))
	// 	std::cout << term->to_string() << '\n';
	// else
	// 	std::cout << "eval failed!\n";

	// std::cout << *evaluate::evaluate_con(*ast);

	// to_string(ast);

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
