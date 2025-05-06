set_project("meshopt")
set_version("0.1.0")

set_arch("x64")
set_plat("windows")
set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
add_rules("plugin.vsxmake.autoupdate")

target("meshopt")
    set_kind("binary")

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")

    add_includedirs("ext/include")
    add_linkdirs("ext/lib")

    add_links("METIS/metis")
target_end()
