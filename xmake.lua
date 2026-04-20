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

    -- Looks for libraries dynamically
    import("lib.detect.find_tool")
    local scanner = find_tool("clang-scan-deps-18") or find_tool("clang-scan-deps")
    if scanner then
        set_values("clang.scan_deps", scanner.program)
    end

    add_cxflags("-stdlib=libc++", "-Wno-reserved-user-defined-literal", {force = true})
    add_ldflags("-stdlib=libc++", {force = true})
    
elseif is_plat("macosx") then
    set_toolchains("llvm")
    set_policy("build.c++.modules.std", true)
end


-- Utility function to find std.cppm on linux
function add_linux_std_module()
    import("lib.detect.find_file")
    -- Looks for the file in all possible paths of different Linux distributions
    local std_module = find_file("std.cppm", {
        "/usr/share/libc++/v1",
        "/usr/lib/llvm-18/include/c++/v1",
        "/usr/lib/llvm-17/include/c++/v1"
    })

    if std_module then
        add_files(std_module, { filetype = "c++.module", headeronly = true })
    else
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
    -- Removed add_files("include/*.cpp") -> ensure your stb_impl.cpp is moved to the src/ directory!

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