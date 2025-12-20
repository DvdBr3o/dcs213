add_requires("matplotplusplus")
-- add_requires("dataframe")
add_requires("rapidcsv")
add_requires("range-v3")
add_requires("clipp")

add_rules("mode.debug", "mode.release")

target("dcs213.project4")
    set_languages("cxx20")

    add_packages("matplotplusplus")
    -- add_packages("dataframe")
    add_packages("rapidcsv")
    add_packages("range-v3")
    add_packages("clipp")

    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp")
    add_includedirs("src")

    after_build(function (target) 
        os.cp(path.join(os.scriptdir(), "public"), target:targetdir())
    end)
