add_requires("stb")
add_requires("yalantinglibs")
add_requires("opencv", {
    configs = {
        ffmpeg = false,
    }
})
add_requires("clipp", {alias = "clipp"})

add_rules("mode.debug", "mode.release")

target("dcs213.project3")
    set_languages("cxx20")

    add_packages("stb", {public = true})
    add_packages("yalantinglibs", {public = true})
    add_packages("opencv", {public = true})
    add_packages("clipp", {public = true})
    
    add_headerfiles("src/**.hpp")
    add_files("src/**.cpp")

    after_build(function (target) 
        os.cp(path.join(os.scriptdir(), "public"), target:targetdir())
    end)
