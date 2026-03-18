-- ==========================================
-- 1. Global settings
-- ==========================================
set_languages("c++23")
set_policy("build.c++.modules", true)

-- ask xmake to automatically download doctest if not already present
add_requires("doctest")

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
-- 2. Principal target (production code)
-- ==========================================
target("PhyxRadpp")
    set_kind("binary")

    if is_plat("linux") then
        add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
    end

    add_files("src/*.cppm")
    add_files("src/*.cpp")

    set_rundir("$(projectdir)") -- runs the tests in the project directory

    -- Remove the test files from the principal directory
    -- (when the Color tests will be addet to the new infrastructure) this line will be removed
    remove_files("src/test_*.cpp")


-- ==========================================
-- 3. Test targets (new infrastructure with doctest)
-- ==========================================
-- This looks for the test files in the "test/" directory (for now only HDRImage)
for _, file in ipairs(os.files("test/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        set_rundir("$(projectdir)") -- The .pfm files and test images will be searched in the principal directory
                                    -- If they are in a sub directory like images/ needs to specify that in the path
                                    -- passed to the functions

        -- The target has to use the doctest library
        add_packages("doctest")

        if is_plat("linux") then
            add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
        end

        -- Compiliamo il test specifico e tutte le tue librerie in src/
        add_files("test/" .. name .. ".cpp")
        add_files("src/*.cppm")

        add_tests("default")
end

-- ==========================================
-- 3. Test targets -- Color (for the Color class, the tests are still in src and don't use doctest)
-- ==========================================
for _, file in ipairs(os.files("src/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false) -- Is not automatically compiled with the xmake command

        set_rundir("$(projectdir)") -- It executes the test in the project directory

        if is_plat("linux") then
            add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
        end

        add_files("src/" .. name .. ".cpp")
        add_files("src/*.cppm") -- test must have the possibility to be modules

        add_tests("default")
end
