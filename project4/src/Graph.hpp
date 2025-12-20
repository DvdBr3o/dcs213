#pragma once

#define NOMINMAX

#include <rapidcsv.h>
#include <limits>
#include <range/v3/view.hpp>
#include <range/v3/range.hpp>
#include <matplot/matplot.h>

#include <filesystem>
#include <unordered_map>
#include <map>
#include <queue>
#include <stack>

namespace dcs213::project4 {
	class Graph {
	public:
		struct Node;

		struct Edge {
			std::size_t	  to;
			std::uint32_t w;
		};

		struct Node {
			std::vector<Edge> es;
			std::string		  name;
		};

		struct DijkState {
			std::size_t		   node;
			std::uint32_t	   dis;

			inline friend auto operator<=>(const DijkState& lhs, const DijkState& rhs) {
				return lhs.dis <=> rhs.dis;
			}
		};

	public:
		Graph(std::size_t n) : _ns(n) {}

		Graph(const std::filesystem::path& map, const std::filesystem::path& city) {
			if (!std::filesystem::exists(map))
				throw std::runtime_error {
					std::format("Failed to open graph at \"{}\"", map.string())
				};
			if (!std::filesystem::exists(city))
				throw std::runtime_error {
					std::format("Failed to open city name at \"{}\"", city.string())
				};

			const auto name = rapidcsv::Document(city.string()).GetColumn<std::string>("name");
			_ns.resize(name.size());
			for (int i = 0; i < _ns.size(); ++i) _ns[i].name = name[i];

			auto	   doc = rapidcsv::Document(map.string());

			const auto src = doc.GetColumn<std::size_t>("src");
			const auto dst = doc.GetColumn<std::size_t>("dst");
			const auto dis = doc.GetColumn<std::uint32_t>("dis");

			for (const auto& [u, v, w] : ranges::views::zip(src, dst, dis)) {
				_add_edge(u, v, w);
				_add_edge(v, u, w);
			}

			std::cout << "graph created!\n";
		}

	public:
		auto show() const {
			using namespace ranges;
			using namespace ranges::views;

			const auto es = views::enumerate(_ns) | transform([](auto&& ns) {
								auto&& [i, node] = ns;
								return node.es | transform([i](const Edge& e) {
										   return std::pair { i, e.to };
									   });
							})
						  | join | to_vector;
			const auto ns	  = _ns | transform([](auto&& ns) { return ns.name; }) | to_vector;
			const auto wlabel = _ns | transform([](const Node& node) {
									return node.es | transform([](const Edge& e) { return e.w; });
								})
							  | join | to_vector;
			const auto ws = wlabel | transform([](auto x) { return std::tanh(x); }) | to_vector;

			for (const auto& [uv, w] : zip(es, wlabel))
				std::cout << std::format(
					"edge({}, {}, {})\n",
					_ns[uv.first].name,
					_ns[uv.second].name,
					w
				);

			auto g = matplot::graph(es);
			g->layout_algorithm(matplot::network::layout::force);
			g->node_labels(ns);
			g->edge_labels(wlabel);
			g->show_labels(true);

			// matplot::view(2);
			matplot::show();
		}

		auto bfs_tree(std::size_t src) const -> Graph {
			Graph g { _ns.size() };
			for (int i = 0; i < _ns.size(); ++i) g._ns[i].name = _ns[i].name;
			std::vector<bool>		vis(_ns.size(), false);

			std::queue<std::size_t> q;
			q.push(src);
			vis[src] = true;
			while (!q.empty()) {
				const auto f = q.front();
				q.pop();
				for (const auto& e : _ns[f].es)
					if (!vis[e.to]) {
						vis[e.to] = true;
						g._add_edge(f, e.to, e.w);
						q.push(e.to);
					}
			}

			return g;
		}

		auto dfs_tree(std::size_t src) const -> Graph {
			Graph g { _ns.size() };
			for (int i = 0; i < _ns.size(); ++i) g._ns[i].name = _ns[i].name;
			std::vector<bool> vis(_ns.size(), false);
			_dfs_vis(src, vis, g);
			return g;
		}

		auto stack_dfs_tree(std::size_t src) const -> Graph {
			Graph g { _ns.size() };
			for (int i = 0; i < _ns.size(); ++i) g._ns[i].name = _ns[i].name;
			std::vector<bool>		vis(_ns.size(), false);

			std::stack<std::size_t> s;
			s.push(src);
			while (!s.empty()) {
				const auto top = s.top();
				bool	   end = true;
				for (const auto e : _ns[top].es)
					if (!vis[e.to]) {
						vis[e.to] = true;
						g._add_edge(top, e.to, e.w);
						s.push(e.to);
						end = false;
						break;
					}
				if (end)
					s.pop();
			}

			return g;
		}

		auto dijkstra(std::size_t src) const -> std::vector<std::uint32_t> {
			std::vector<std::uint32_t> dis(_ns.size(), std::numeric_limits<std::uint32_t>::max());
			std::vector<bool>		   vis(_ns.size(), false);
			std::priority_queue<DijkState, std::vector<DijkState>, std::greater<>> q;
			dis[src] = 0;

			q.push({ src, 0 });

			while (!q.empty()) {
				const auto f = q.top();
				q.pop();
				if (vis[f.node])
					continue;
				vis[f.node] = true;
				dis[f.node] = f.dis;
				for (const auto& e : _ns[f.node].es)
					if (dis[e.to] > dis[f.node] + e.w) {
						dis[e.to] = dis[f.node] + e.w;
						q.push({ e.to, dis[f.node] + e.w });
					}
			}

			return dis;
		}

		auto print_dijk(std::size_t src, const std::vector<std::uint32_t>& dis) const {
			using namespace ranges::views;
			for (const auto [i, d] : enumerate(dis))
				std::cout << std::format("dis({} -> {}) = {}\n", _ns[src].name, _ns[i].name, d);
		}

	private:
		void _add_edge(std::size_t from, std::size_t to, std::uint32_t dis) {
			_ns[from].es.emplace_back(to, dis);
		}

		void _dfs_vis(std::size_t cur, std::vector<bool>& vis, Graph& g) const {
			for (const auto& e : _ns[cur].es) {
				if (!vis[e.to]) {
					vis[e.to] = true;
					g._add_edge(cur, e.to, e.w);
					_dfs_vis(e.to, vis, g);
				}
			}
		}

	private:
		std::vector<Node> _ns;
	};
}  // namespace dcs213::project4