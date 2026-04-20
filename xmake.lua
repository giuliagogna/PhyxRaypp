-- ==========================================
-- 1. Global settings
-- ==========================================
set_languages("c++23")
set_policy("build.c++.modules", true)

-- Ask xmake to automatically download doctest and stb if they are not already present
add_requires("doctest")
add_requires("stb")

if is_plat("linux") then
    set_toolchains("clang")
    set_policy("build.c++.modules.std", false)

    -- Dynamically search for the scanner using known system paths without invoking 'import'
    local scanners = {
        "/usr/bin/clang-scan-deps-18",
        "/usr/bin/clang-scan-deps"
    }
    for _, scanner in ipairs(scanners) do
        if os.isfile(scanner) then
            set_values("clang.scan_deps", scanner)
            break
        end
    end

    add_cxflags("-stdlib=libc++", "-Wno-reserved-user-defined-literal", {force = true})
    add_ldflags("-stdlib=libc++", {force = true})

elseif is_plat("macosx") then
    set_toolchains("llvm")
    set_policy("build.c++.modules.std", true)
end

-- ==========================================
-- Utility function to find std.cppm on Linux
-- ==========================================
function add_linux_std_module()
    -- Look for the file by safely iterating over known paths of different Linux distributions
    local std_paths = {
        "/usr/share/libc++/v1/std.cppm",            -- Arch Linux / Custom
        "/usr/lib/llvm-18/include/c++/v1/std.cppm", -- Ubuntu 24.04 (Clang 18)
        "/usr/lib/llvm-17/include/c++/v1/std.cppm"  -- Ubuntu 22.04 (Clang 17)
    }

    local found = false
    for _, p in ipairs(std_paths) do
        if os.isfile(p) then
            add_files(p, { filetype = "c++.module", headeronly = true })
            found = true
            break
        end
    end

    if not found then
        print("WARNING: std.cppm not found! You need to install libc++-dev.")
    end
end

-- ==========================================
-- 2. Principal target (production code)
-- ==========================================
target("PhyxRadpp")
    set_kind("binary")

    -- FIX 1: The main target needs the stb package to link properly
    add_packages("stb")

    if is_plat("linux") then
        add_linux_std_module()
    end

    add_files("src/*.cppm")
    add_files("src/*.cpp")

    set_rundir("$(projectdir)") 

    -- Exclude the test files from the main target
    remove_files("src/test_*.cpp")

-- ==========================================
-- 3. Test targets (new infrastructure with doctest)
-- ==========================================
for _, file in ipairs(os.files("test/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false)
        set_rundir("$(projectdir)") 

        -- Ensure the target uses the doctest and stb packages
        add_packages("doctest", "stb")

        if is_plat("linux") then
            add_linux_std_module()
        end

        add_files("test/" .. name .. ".cpp")
        add_files("src/*.cppm")
        
        -- FIX 2: Compile the source files in src/ (including stb_impl.cpp) 
        -- otherwise the test executable won't find the stb implementation!
        add_files("src/*.cpp")
        
        -- Prevent compiling the program's main file and the old tests into this target
        remove_files("src/test_*.cpp")
        remove_files("src/main.cpp") -- IMPORTANT: if your main entry point is named differently, update this line

        add_tests("default")
end