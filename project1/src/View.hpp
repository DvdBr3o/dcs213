#pragma once

#include "Utils.hpp"

#if defined DCS213_P1_PLAT_WINDOWS
#	define GLFW_EXPOSE_NATIVE_WIN32
#elif defined DCS213_P1_PLAT_MACOS
#	define GLFW_EXPOSE_NATIVE_COCOA
#elif defined DCS213_P1_PLAT_LINUX
#	define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <webview/webview.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <thread>
#include <iostream>
#include <format>

namespace dcs213::p1 {
	class MainView : public webview::webview {
	public:
		struct Spec {
			bool		debug  = false;
			int			width  = 400;
			int			height = 600;
			std::string title  = "Calculator";
			std::string ui;
		};

	private:
		inline auto _window_from(const Spec& spec) -> void*;

		class GlfwManager {
		public:
			~GlfwManager() { glfwTerminate(); }

			inline static auto& init() {
				static GlfwManager manager;
				return manager;
			}

		private:
			GlfwManager() {
				if (!glfwInit())
					throw std::runtime_error { "Failed to init glfw!" };
			}
		};

	public:
		MainView(const Spec& spec);

		MainView() : MainView(Spec {}) {}

	public:
		auto launch() -> void;

		/**
		 * @brief Bind a function to the webview.
		 *
		 * unlike `webview::webview::bind`, this function does the json arg parsing automatically
		 * for you.
		 *
		 * @see webview::webview::bind
		 *
		 * @tparam Args
		 * @param name
		 * @param f
		 * @return webview::noresult
		 */
		template<typename... Args>
		auto bind_fn(
			const std::string&			   name,
			std::invocable<Args...> auto&& f
		) noexcept(std::is_nothrow_invocable_v<decltype(f), Args...>) -> webview::noresult {
			return this->webview::webview::bind(
				name,
				[&](const std::string& id, const std::string& req, void* /* arg */) -> void {
					// Parse `req` to args and send to `f`.
					using R = std::invoke_result_t<decltype(f), Args...>;
					std::tuple<Args...> args;
					json::init_args(args, req);

					if constexpr (std::is_void_v<R>) {
						std::apply(f, args);
						resolve(id, 0, "");
					} else {
						auto result = std::apply(f, args);
						// TODO: General `result -> json string` abstraction.
						// TODO: Fix string -> json string conversion, e.g. `\` -> `\\` in strings.
						// resolve(id, 0, std::to_string(result));
						// resolve(id, 0, std::format("\"{}\"", result));
						std::cerr << result;
						// resolve(id, 0, result);
						resolve(id, 0, result);
					}
				},
				nullptr
			);
		}

	private:
		GLFWwindow* _window;
	};

	inline static constexpr auto ui = R"html(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Calculator</title>
    <style>
      *,
      *::before,
      *::after {
        box-sizing: border-box;
      }

      :root {
        --background-color: rgb(255, 255, 255);
        --pad-color: rgb(241, 253, 255);
        --text-color: rgb(26, 26, 27);

        @media (prefers-color-scheme: dark) {
          --background-color: rgb(0, 0, 0);
          --pad-color: rgb(26, 26, 27);
          --text-color: rgb(241, 253, 255);
        }
      }

      [data-theme="light"] {
        --background-color: rgb(255, 255, 255);
        --pad-color: rgb(241, 253, 255);
        --text-color: rgb(26, 26, 27);
      }

      [data-theme="dark"] {
        --background-color: rgb(0, 0, 0);
        --pad-color: rgb(26, 26, 27);
        --text-color: rgb(241, 253, 255);
      }

      body {
        padding-left: 5px;
        padding-right: 5px;
        padding-bottom: 10px;
        overflow: hidden;
        min-width: min-content;
        min-height: min-content;
        background-color: var(--background-color);
      }

      .calculator-box {
        display: flex;
        flex-direction: column;
        align-items: center;
        padding-top: 15px;
        padding-left: 10px;
        padding-right: 10px;
        padding-bottom: 15px;
        width: 90vw;
        min-width: min-content;
        max-width: min(95vw, calc(100vh / 1.45));
        max-height: 95vh;
        aspect-ratio: 1 / 1.45;
        justify-self: center;
        align-self: center;
      }

      .calculator-display {
        border-radius: 20px;
        width: 100%;
        height: 25%;
        padding: 15px;
        background-color: var(--pad-color);
        color: var(--text-color);
        text-align: right;
        align-items: center;
        overflow: hidden;
        white-space: nowrap;
        text-overflow: ellipsis;
        flex-shrink: 0;
      }
      .calculator-display-text {
        font-size: 24px;
        font-family: "HarmonyOS Sans SC";
        overflow: hidden;
        white-space: nowrap;
        text-overflow: ellipsis;
      }
      .calculator-display-input {
        border: none;
        outline: none;
        font-size: 24px;
        font-family: "HarmonyOS Sans SC";
        overflow: hidden;
        white-space: nowrap;
        text-overflow: ellipsis;
        background: none;
        text-align: right;
        width: 100%;
        height: 100%;
        color: var(--text-color);

        &:focus {
          outline: none;
        }
      }

      .calculator-button-box {
        display: grid;
        grid-template-columns: 25%;
        justify-content: stretch;
        align-content: stretch;
        grid-template-columns: repeat(5, 1fr);
        grid-template-rows: repeat(5, 1fr);
        padding-top: 20px;
        padding-left: 0;
        padding-right: 0;
        padding-bottom: 20px;
        width: 100%;
        gap: 10px;
        box-sizing: border-box;
        flex-shrink: 0;
        flex-grow: 1;
        min-width: min-content;
        margin-left: 10px;
        margin-right: 10px;
      }

      .calculator-button-number {
        width: 100%;
        height: 100%;
        padding: 0;
        border: none;
        background: none;
        outline: none;
        align-self: center;
        justify-self: center;
        padding: 0.3em;
        min-height: 0;
      }

      .calculator-button-number-padding {
        position: relative;
        z-index: 1;
        width: 100%;
        height: 100%;
        aspect-ratio: 1 / 1;
        border-radius: 30%;
        background-color: var(--pad-color);
        justify-items: center;
        justify-content: center;
        align-items: center;
        align-content: center;
        text-align: center;
        font-size: 28px;
        font-family: "HarmonyOS Sans SC";
        min-height: fit-content;
        transition: 0.2s ease;

        &:hover {
          transform: scale(1.05);
          filter: brightness(110%);
          box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
        }

        &:active {
          transform: scale(0.95);
          filter: brightness(130%);
        }
      }

      .calculator-button-number-padding-text {
        color: var(--text-color);
        justify-self: center;
        align-self: center;
        margin: auto;
      }
    </style>
  </head>
  <body>
    <div class="calculator-box">
      <div class="calculator-display">
        <div class="calculator-display-text">
          <input
            type="text"
            placeholder="&#x8F93;&#x5165;&#x8868;&#x8FBE;&#x5F0F;"
            class="calculator-display-input"
            id="displayInput"
          />
        </div>
      </div>
      <div class="calculator-button-box">
        <button class="calculator-button-number" id="dayNightSwitcher">
          <div class="calculator-button-number-padding">
            <div
              class="calculator-button-number-padding-text"
              id="dayNightSwitcherText"
            >
              &#x2600;&#xFE0F;
            </div>
          </div>
        </button>
        <button class="calculator-button-number" id="allClear">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">AC</div>
          </div>
        </button>
        <button class="calculator-button-number" id="backSpace">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">
              &#x21A9;&#xFE0F;
            </div>
          </div>
        </button>
        <button class="calculator-button-number" id="opDevide">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">&#xF7;</div>
          </div>
        </button>
        <button class="calculator-button-number" id="constantE">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">e</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num7">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">7</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num8">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">8</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num9">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">9</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opMultiply">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">&#xD7;</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opLn">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">ln</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num4">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">4</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num5">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">5</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num6">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">6</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opMinus">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">-</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opExponent">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">^</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num1">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">1</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num2">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">2</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num3">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">3</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opPlus">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">+</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opLParen">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">(</div>
          </div>
        </button>
        <button class="calculator-button-number" id="blank">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">SP</div>
          </div>
        </button>
        <button class="calculator-button-number" id="num0">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">0</div>
          </div>
        </button>
        <button class="calculator-button-number" id="numDot">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">.</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opEqual">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">=</div>
          </div>
        </button>
        <button class="calculator-button-number" id="opRParen">
          <div class="calculator-button-number-padding">
            <div class="calculator-button-number-padding-text">)</div>
          </div>
        </button>
      </div>
    </div>
    <script type="module">
      const getElements = (ids) =>
        Object.assign(
          {},
          ...ids.map((id) => ({ [id]: document.getElementById(id) }))
        );
      const ui = getElements([
        "displayInput",
        "dayNightSwitcher",
        "dayNightSwitcherText",
        "allClear",
        "backSpace",
        "opDevide",
        "constantE",
        "num7",
        "num8",
        "num9",
        "opMultiply",
        "opLn",
        "num4",
        "num5",
        "num6",
        "opMinus",
        "opExponent",
        "num1",
        "num2",
        "num3",
        "opPlus",
        "opLParen",
        "blank",
        "num0",
        "numDot",
        "opEqual",
        "opRParen",
      ]);

      const toggleTheme = async () => {
        const currentTheme =
          document.documentElement.getAttribute("data-theme");
        const newTheme = currentTheme === "dark" ? "light" : "dark";
        document.documentElement.setAttribute("data-theme", newTheme);
        localStorage.setItem("theme", newTheme);
        if (newTheme === "dark") {
          ui.dayNightSwitcherText.textContent = "\u{1F319}";
        } else {
          ui.dayNightSwitcherText.textContent = "\u{2600}\u{FE0F}";
        }
      };

      document.addEventListener("DOMContentLoaded", async () => {
        const savedTheme = localStorage.getItem("theme") || "light";
        document.documentElement.setAttribute("data-theme", savedTheme);
        if (savedTheme) {
          document.documentElement.setAttribute("data-theme", savedTheme);
          if (savedTheme === "dark") {
            ui.dayNightSwitcherText.textContent = "\u{1F319}";
          } else {
            ui.dayNightSwitcherText.textContent = "\u{2600}\u{FE0F}";
          }
        } else {
          const prefersDark = window.matchMedia(
            "(prefers-color-scheme: dark)"
          ).matches;
          document.documentElement.setAttribute(
            "data-theme",
            prefersDark ? "dark" : "light"
          );
          if (prefersDark === true) {
            ui.dayNightSwitcherText.textContent = "\u{1F319}";
          } else {
            ui.dayNightSwitcherText.textContent = "\u{2600}\u{FE0F}";
          }
        }
      });
      ui.dayNightSwitcher.addEventListener("click", async () => {
        toggleTheme();
      });

      const buttonMap = {
        numDot: ".",
        blank: " ",
        num0: "0",
        num1: "1",
        num2: "2",
        num3: "3",
        num4: "4",
        num5: "5",
        num6: "6",
        num7: "7",
        num8: "8",
        num9: "9",
        opPlus: "+",
        opMinus: "-",
        opMultiply: "*",
        opDevide: "/",
        opExponent: "^",
        opLn: "ln",
        opLParen: "(",
        opRParen: ")",
        constantE: "e",
      };

      for (const [k, v] of Object.entries(buttonMap)) {
        ui[k].addEventListener("click", async () => {
          const display = ui.displayInput;
          display.value = display.value + v;
          const newLength = display.value.length;
          display.selectionStart = newLength;
          display.selectionEnd = newLength;
          display.focus();
        });
      }

      ui.backSpace.addEventListener("click", async () => {
        ui.displayInput.value = ui.displayInput.value.slice(0, -1);
      });

      ui.displayInput.addEventListener("keyup", async (event) => {
        if (event.key === "Enter") {
          event.preventDefault();
          const inputValue = event.target.value;
          console.log(inputValue);
        }
      });

      ui.allClear.addEventListener("click", async () => {
        ui.displayInput.value = "";
        displayInput.selectionStart = 0;
        displayInput.selectionEnd = 0;
        displayInput.focus();
      });
    </script>
  </body>
</html>
)html";
}  // namespace dcs213::p1

namespace dcs213::p1 {
	inline auto MainView::_window_from(const Spec& spec) -> void* {
		GlfwManager::init();

		// glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		// glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		auto* _window =
			glfwCreateWindow(spec.width, spec.height, spec.title.data(), nullptr, nullptr);
		if (!_window)
			throw std::runtime_error { "Failed to create window!" };

		glfwSetWindowUserPointer(_window, this);
		// glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height) {
		// 	auto& self = *static_cast<MainView*>(glfwGetWindowUserPointer(window));
		// 	self.set_size(width, height, WEBVIEW_HINT_NONE);
		// });

#if defined DCS213_P1_PLAT_WINDOWS
		return glfwGetWin32Window(_window);
#elif defined DCS213_P1_PLAT_MACOS
		return glfwGetCocoaWindow(_window);
#elif defined DCS213_P1_PLAT_LINUX
		return (void*)(uintptr_t)glfwGetWaylandWindow(_window);
#endif
	}

	inline MainView::MainView(const Spec& spec) : webview::webview(spec.debug, nullptr) {
		bind_fn("terminate", [this]() {
			std::cerr << std::format("Received terminate request!");
			this->terminate();
		});

		set_title(spec.title);
		set_size(spec.width, spec.height, WEBVIEW_HINT_NONE);
		set_html(spec.ui);
	}

	inline auto MainView::launch() -> void {
		run();
	}
}  // namespace dcs213::p1