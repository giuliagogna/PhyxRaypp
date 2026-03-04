-- ==========================================
-- 1. Global settings
-- ==========================================
set_languages("c++23")
set_policy("build.c++.modules", true)

if is_plat("linux") then
    set_toolchains("clang")
    set_policy("build.c++.modules.std", false)
    set_values("clang.scan_deps", "/usr/bin/clang-scan-deps")
    add_cxflags("-stdlib=libc++", "-Wno-reserved-user-defined-literal", {force = true})
    add_ldflags("-stdlib=libc++", {force = true})

elseif is_plat("macosx") then
    set_toolchains("llvm")
    set_policy("build.c++.modules.std", true)
end

-- ==========================================
-- 2. Principal target
-- ==========================================
target("PhyxRadpp")
    set_kind("binary")

    if is_plat("linux") then
        add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
    end

    add_files("src/*.cppm")
    add_files("src/*.cpp")

    -- VERY IMPORTANT: tells the principal target not to compile the tests
    remove_files("src/test_*.cpp")


-- ==========================================
-- 3. Test targets
-- ==========================================
for _, file in ipairs(os.files("src/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false) -- Is not automatically compiled with the xmake command

        if is_plat("linux") then
            add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
        end

        add_files("src/" .. name .. ".cpp")
        add_files("src/*.cppm") -- test must have the possibility to be modules

        add_tests("default")
end
