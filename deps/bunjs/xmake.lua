package("bun")
    set_kind("binary")
    set_homepage("https://bun.sh/")
    set_description("Incredibly fast JavaScript runtime, bundler, test runner, and package manager – all in one")
    set_license("MIT")

    on_install("@linux", "@macosx", "@windows", function(package)
        if package:is_plat("windows") then
            os.vrun("powershell -c \"irm bun.sh/install.ps1|iex\"")
        else
            os.vrunv("curl -fsSL https://bun.sh/install | bash")
        end
    end)

    on_test(function(package)
        os.vrun("bun --version")
    end)
package_end()
