#pragma once

#include <clipp.h>

#include <iostream>
#include "Graph.hpp"

namespace dcs213::project4 {
	class CommandLine {
	public:
		enum class Mode {
			Help,
			Dfs,
			StackDfs,
			Bfs,
			Dis,
		};

	public:
		CommandLine(int argc, char** argv) {
			using namespace clipp;

			const auto src	= (required("--src", "-s"), value("src").set(_src));
			const auto map	= (required("--map", "-m"), value("map").set(_map));
			const auto name = (required("--name", "-n"), value("name").set(_name));

			const auto help =
				command("help", "--help", "-h").set(_mode, Mode::Help) % "Show help message";
			const auto dfs		= (command("dfs").set(_mode, Mode::Dfs), src, map, name);
			const auto stackdfs = (command("stackdfs").set(_mode, Mode::StackDfs), src, map, name);
			const auto bfs		= (command("bfs").set(_mode, Mode::Bfs), src, map, name);
			const auto dis		= (command("dis").set(_mode, Mode::Dis), src, map, name);

			const auto cli		= (help | dfs | stackdfs | bfs | dis);

			if (!parse(argc, argv, cli))
				std::cout << make_man_page(cli);

			if (_mode == Mode::Help)
				std::cout << make_man_page(cli);
			else if (_mode == Mode::Dfs) {
				const auto g = Graph { _map, _name };
				const auto d = g.dfs_tree(_src);
				g.show();
				d.show();
			} else if (_mode == Mode::StackDfs) {
				const auto g = Graph { _map, _name };
				const auto d = g.stack_dfs_tree(_src);
				g.show();
				d.show();
			} else if (_mode == Mode::Bfs) {
				const auto g = Graph { _map, _name };
				const auto d = g.bfs_tree(_src);
				g.show();
				d.show();
			} else if (_mode == Mode::Dis) {
				const auto g	= Graph { _map, _name };
				const auto dist = g.dijkstra(_src);
				g.print_dijk(_src, dist);
				g.show();
			}
		}

	private:
		Mode		_mode;
		std::string _map;
		std::string _name;
		std::size_t _src;
	};
}  // namespace dcs213::project4