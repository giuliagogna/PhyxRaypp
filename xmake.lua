set_languages("c++23")
set_policy("build.c++.modules", true)

target("PhyxRadpp")
    set_kind("binary")

    if is_plat("linux") then
        -- ===== FEDORA / LINUX CONFIG =====
        set_toolchains("clang")

        set_policy("build.c++.modules.std", false)

        set_values("clang.scan_deps", "/usr/bin/clang-scan-deps")

        add_cxflags("-stdlib=libc++", "-Wno-reserved-user-defined-literal", {force = true})
        add_ldflags("-stdlib=libc++", {force = true})

        add_files("/usr/share/libc++/v1/std.cppm", {
            filetype = "c++.module",
            headeronly = true
        })

    elseif is_plat("macosx") then
        -- ===== MACOS CONFIG (semplice) =====
        set_toolchains("llvm")
        -- NIENTE hack
        -- Apple Clang ha già std modulare pronto: (Giulia utente mac) questo è falso, non sono pronte
    end

    add_files("src/*.cppm")
    add_files("src/*.cpp")
--

