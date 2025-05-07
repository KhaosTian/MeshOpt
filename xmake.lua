set_project("Nanite")
set_version("0.1.0")

set_arch("x64")
set_plat("windows")
set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
add_rules("plugin.vsxmake.autoupdate")

target("Nanite")
    set_kind("binary")

    add_files("source/**.cpp")
    add_headerfiles("source/**.hpp")
    add_headerfiles("source/**.h")

    add_includedirs("source")
    add_includedirs("external/include")
    add_linkdirs("external/lib")

    add_links("METIS/metis")
target_end()
