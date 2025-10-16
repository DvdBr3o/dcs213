add_requires("webview")
add_requires("glfw")
add_requires("simdjson")
add_requires("tl_expected")
add_requires("magic_enum")

includes("ui")

target("dcs213.project1")
    set_languages("cxx20")

    add_packages("webview", {public = true})
    add_packages("glfw", {public = true})
    add_packages("simdjson", {public = true})
    add_packages("tl_expected", {public = true})
    add_packages("magic_enum", {public = true})
    add_deps("dcs213.project1.ui")
    
    add_headerfiles("src/**.hpp")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})

    if is_plat("windows") then
        add_defines("DCS213_P1_PLAT_WINDOWS", {public = true})
    elseif is_plat("macos") then
        add_defines("DCS213_P1_PLAT_MACOS", {public = true})
    elseif is_plat("linux") then
        add_defines("DCS213_P1_PLAT_LINUX", {public = true})
    end