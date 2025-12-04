#pragma once

#include "Utils.hpp"

#include <chrono>
#include <ylt/struct_pack.hpp>
#include <opencv2/opencv.hpp>

#include <stdexcept>
#include <iterator>
#include <fstream>
#include <queue>
#include <numeric>
#include <map>

namespace dcs213::project3 {
	struct HuffmanChannel {
		struct HuffmanVal {
			std::uint32_t  len;
			std::uint32_t  huff;

			constexpr auto operator*() const -> std::uint32_t { return ((1 << len) - 1) & huff; }

			[[nodiscard]] constexpr auto to_string() const -> std::string {
				if (len == 0)
					return "NUL";
				std::string s;
				for (int i = 0; i < len; ++i) s += ((1 << (len - i - 1)) & huff) ? '1' : '0';
				return s;
			}

			inline friend auto operator==(const HuffmanVal& l, const HuffmanVal& r) {
				return l.len == r.len && *l == *r;
			}

			inline friend auto operator<(const HuffmanVal& l, const HuffmanVal& r) {
				return l.len != r.len ?
						   l.len < r.len :
						   (l.huff & ((1 << l.len) - 1)) < (r.huff & ((1 << r.len) - 1));
			}

			[[nodiscard]] auto cat(bool b) const -> HuffmanVal {
				return { len + 1, (huff << 1) | b };
			}
		};

		struct Freq {
			int							 v = -1;
			std::size_t					 freq;

			inline constexpr friend auto operator<=>(const Freq& l, const Freq& r) {
				return l.freq <=> r.freq;
			}

			inline constexpr friend auto operator+(const Freq& l, const Freq& r) {
				return Freq { -1, l.freq + r.freq };
			}
		};

		struct HuffmanNode {
			Freq			   f;
			const HuffmanNode* l = nullptr;
			const HuffmanNode* r = nullptr;

			//
			void gen_map(HuffmanChannel& map, std::uint32_t len = 0, std::uint32_t code = 0) const {
				if (l)
					l->gen_map(map, len + 1, ((code << 1) | std::uint32_t(0)));
				if (r)
					r->gen_map(map, len + 1, ((code << 1) | std::uint32_t(1)));
				if (f.v >= 0)
					map.map[f.v] = { len, (code & ((1 << len) - 1)) };
			}

			int huf(const HuffmanVal& val, int cur = 0) const {
				if (cur == val.len)
					return f.v;
				if (val.huff & (1 << (val.len - cur)))
					return l->huf(val, cur + 1);
				else
					return r->huf(val, cur + 1);
			}
		};

		struct QueueNode {
			const HuffmanNode*			 p;

			inline constexpr friend auto operator<=>(const QueueNode& l, const QueueNode& r) {
				return l.p->f <=> r.p->f;
			}

			constexpr auto* operator->() const { return p; }
		};

		struct DecodeStream {
			std::uint32_t				maxb = 0;
			std::map<HuffmanVal, uchar> map;
			std::span<const uchar>		data;
			std::size_t					b	= 0;
			std::size_t					inb = 0;

			DecodeStream(const HuffmanChannel& chan) : data(chan.data) {
				// std::cout << std::format(
				// 	"checking {} vs {}: {} because {} vs {} && {} vs {}\n",
				// 	HuffmanVal { 5, 0b00110 }.to_string(),
				// 	chan.map[106].to_string(),
				// 	(HuffmanVal { 5, 0b00110 } == chan.map[106]),
				// 	HuffmanVal { 5, 0b00110 }.len,
				// 	chan.map[106].len,
				// 	HuffmanVal { 5, 0b00110 }.huff,
				// 	chan.map[106].huff
				// );
				for (int i = 0; i < chan.map.size(); ++i) {
					map[chan.map[i]] = i;
					maxb			 = std::max(maxb, chan.map[i].len);
					// std::cout << std::format("{} {:16}\n", i, chan.map[i].to_string());
				}
			}

			auto bump() -> uchar {
				// std::cout << "bumping!\n";
				if (b >= data.size()) {
					std::cout
						<< std::format("exceeding: b = {}, data.size() = {}\n", b, data.size());
					return 0;
				}
				HuffmanVal huff = {};
				for (std::uint32_t l = 1; l <= maxb; ++l) {
					if (b >= data.size())
						return 0;
					huff = huff.cat(data[b] & (1 << (8 - inb - 1)));
					++inb;
					b += inb / 8;
					inb %= 8;
					// std::cout << std::format("finding: {}\n", huff.to_string());
					auto it = map.find(huff);
					if (it != map.end()) {
						static int count = 0;
						if (count < 2) {
							++count;
							// std::cout << std::format(
							// 	"decode raw data: {} -> {}\n",
							// 	huff.to_string(),
							// 	it->second
							// );
						}
						return it->second;
					}
				}
				return 0;
			}
		};

		std::array<HuffmanVal, 256> map {};
		std::vector<uchar>			data;

		constexpr auto&				operator[](int c) { return map[c]; }

		constexpr const auto&		operator[](int c) const { return map[c]; }

		//
		inline static auto from(const cv::Mat& mat) -> HuffmanChannel {
			HuffmanChannel		  chan;

			std::array<Freq, 256> freq = { 0 };
			for (int i = 0; i < freq.size(); ++i) freq[i].v = i;
			for (int i = 0; i < mat.size[0]; ++i)
				for (int j = 0; j < mat.size[1]; ++j) ++freq[mat.at<uchar>(i, j)].freq;
			std::vector<HuffmanNode> ns;
			ns.reserve(256 * 2);
			for (const auto& f : freq) ns.emplace_back(f, nullptr, nullptr);
			std::priority_queue<QueueNode, std::vector<QueueNode>, std::greater<>> q {};
			for (const auto& n : ns) q.emplace(&n);
			while (q.size() > 1) {
				auto l = q.top();
				q.pop();
				auto r = q.top();
				q.pop();
				ns.emplace_back(l->f + r->f, l.p, r.p);
				q.emplace(&ns.back());
			}
			q.top()->gen_map(chan);

			// for (int i = 0; i < 256; ++i)
			// 	std::cout << std::format("{} {} {}\n", i, ns[i].f.freq, chan.map[i].to_string());

			std::vector<bool> bitstream;
			bitstream.reserve(mat.size[0] * mat.size[1]);
			// std::cout << std::format("raw: {} {}\n", mat.at<uchar>(0, 0), mat.at<uchar>(0, 1));
			for (int i = 0; i < mat.size[0]; ++i)
				for (int j = 0; j < mat.size[1]; ++j) {
					// std::cout << std::format("at {}, {}\n", i, j);
					const auto huf = chan[mat.at<uchar>(i, j)];
					for (int i = 0; i < huf.len; ++i)
						bitstream.emplace_back(((1 << (huf.len - i - 1)) & huf.huff));
				}
			// std::cout << "bs: ";
			// for (int i = 0; i < 8; ++i) std::cout << bitstream[i];
			// std::cout << ' ';
			// for (int i = 0; i < 8; ++i) std::cout << bitstream[8 + i];
			// std::cout << '\n';

			chan.data.reserve(bitstream.size() / 8 + 1);

			// int	  cnt = 0;
			// uchar by  = 0;
			// for (const auto b : bitstream) {
			// 	if (cnt >= 8) {
			// 		chan.data.emplace_back(by);
			// 		cnt = 0;
			// 		by	= 0;
			// 	}
			// 	by |= (b << (8 - cnt - 1));
			// }
			// if (cnt < 8)
			// 	chan.data.emplace_back(by);

			for (int b = 0; b <= bitstream.size() / 8; ++b) {
				auto curby = uchar(0);
				for (int i = 0; i < 8; ++i)
					if (8 * b + i >= bitstream.size())
						break;
					else
						curby |= (bitstream[8 * b + i] << (8 - i - 1));
				chan.data.emplace_back(curby);
			}

			return chan;
		}
	};

	struct HuffmanPack {
		int							width;
		int							height;
		std::vector<HuffmanChannel> channels;

		//
		inline static auto from(const cv::Mat& mat) -> HuffmanPack {
			HuffmanPack pack;

			pack.width	= mat.size[1];
			pack.height = mat.size[0];
			std::vector<cv::Mat> channs;
			cv::split(mat, channs);
			pack.channels.reserve(channs.size());
			for (auto&& c : channs) {
				pack.channels.emplace_back(HuffmanChannel::from(c));
				// std::cout << std::format("data size: {}\n", pack.channels.back().data.size());
				// std::cout << std::format(
				// 	"data: {:08b} {:08b}\n",
				// 	pack.channels.back().data[0],
				// 	pack.channels.back().data[1]
				// );	//* fine
			}

			return pack;
		}
	};

	inline static auto encode_halffman(const std::string& img_path, const std::string& to_path) {
		auto	   start = std::chrono::high_resolution_clock::now();
		const auto img	 = cv::imread(img_path);
		const auto pack	 = HuffmanPack::from(img);
		auto	   ser	 = struct_pack::serialize(pack);

		bin_to(to_path, ser);

		cv::imshow(img_path, img);

		std::cout << "encode done!\n";
		std::cout << std::format(
			"encode time usage: {}\n",
			std::chrono::high_resolution_clock::now() - start
		);
		std::cout << std::format(
			"compress ratio: {:.2}%\n",
			static_cast<double>(ser.size()) / (img.size[0] * img.size[1] * img.channels()) * 100.
		);

		return ser;
	}

	inline static auto decode_halffman(std::span<const char> data, const std::string& to_path) {
		auto	   start = std::chrono::high_resolution_clock::now();

		const auto pack	 = struct_pack::deserialize<HuffmanPack>(data);
		if (!pack)
			throw std::runtime_error {
				"Failed to deserialize file! Maybe file is broken or not in huffman code!"
			};
		// std::cout << std::format("size: {}, {}\n", pack->width, pack->height);
		// std::cout << std::format("num_channel: {}\n", pack->channels.size());

		if (pack->channels.size() == 1) {
			auto img = cv::Mat(pack->height, pack->width, CV_8U);
			std::vector<HuffmanChannel::DecodeStream> ds;
			ds.reserve(pack->channels.size());
			for (auto&& c : pack->channels) ds.emplace_back(c);
			for (int i = 0; i < pack->height; ++i)
				for (int j = 0; j < pack->width; ++j) {
					// std::cout << std::format("at {}, {}\n", i, j);
					img.at<uchar>(i, j) = ds[0].bump();
				}
			cv::imwrite(to_path, img);
			cv::imshow(to_path, img);
		} else if (pack->channels.size() == 3) {
			auto img = cv::Mat(pack->height, pack->width, CV_8UC3);
			std::vector<HuffmanChannel::DecodeStream> ds;
			ds.reserve(pack->channels.size());
			for (auto&& c : pack->channels) ds.emplace_back(c);
			for (int i = 0; i < pack->height; ++i)
				for (int j = 0; j < pack->width; ++j) {
					// std::cout << std::format("at {}, {}\n", i, j);
					const auto r = ds[0].bump();
					const auto g = ds[1].bump();
					const auto b = ds[2].bump();
					// std::cout << "after bump\n";
					// if (!r || !g || !b)
					// 	goto last;
					img.at<cv::Vec3b>(i, j) = {
						r,
						g,
						b,
					};
				}
			// last:
			// std::cout << std::format(
			// 	"decode data: {} {}\n",
			// 	img.at<cv::Vec3b>(0, 0)[0],
			// 	img.at<cv::Vec3b>(0, 1)[0]
			// );
			cv::imwrite(to_path, img);
			cv::imshow(to_path, img);
		} else if (pack->channels.size() == 4) {
			auto img = cv::Mat(pack->height, pack->width, CV_8UC4);
			std::vector<HuffmanChannel::DecodeStream> ds;
			ds.reserve(pack->channels.size());
			for (auto&& c : pack->channels) ds.emplace_back(c);
			for (int i = 0; i < pack->height; ++i)
				for (int j = 0; j < pack->width; ++j) {
					// std::cout << std::format("at {}, {}\n", i, j);
					img.at<cv::Vec4b>(i, j) = {
						ds[0].bump(),
						ds[1].bump(),
						ds[2].bump(),
						ds[3].bump(),
					};
				}
			cv::imwrite(to_path, img);
			cv::imshow(to_path, img);
		}
		std::cout << "decode done!\n";
		std::cout << std::format(
			"decode time usage: {}\n",
			std::chrono::high_resolution_clock::now() - start
		);
		cv::waitKey();
	}

	inline static auto decode_halffman(const std::string& img_path, const std::string& to_path) {
		decode_halffman(bin_from(img_path), to_path);
	}

}  // namespace dcs213::project3