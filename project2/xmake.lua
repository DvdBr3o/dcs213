-- add_requires("opencv")
add_requires("vcpkg::clipp", {public = true})

target("dcs213.project2")
    set_kind("binary")
    set_languages("cxx20")

    add_packages("opencv", {public = true})
    add_packages("clipp", {public = true})

    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp")
    add_includedirs("src", {public = true})
    
    after_build(function (target)
        os.cp("$(scriptdir)/public/**", target:targetdir())
    end)