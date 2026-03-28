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

    -- FIX 1: The main target needs the stb package to link properly
    add_packages("stb")

    if is_plat("linux") then
        add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
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
            add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
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

-- ==========================================
-- 4. Test targets -- Color (old infrastructure)
-- ==========================================
for _, file in ipairs(os.files("src/test_*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        set_default(false) 

        set_rundir("$(projectdir)") 

        -- FIX 3: Added stb to the old tests in case they need to save images during execution
        add_packages("stb")

        if is_plat("linux") then
            add_files("/usr/share/libc++/v1/std.cppm", { filetype = "c++.module", headeronly = true })
        end

        add_files("src/*.cppm") 
        
        -- Include all source files along with the specific test file being processed
        add_files("src/*.cpp")
        
        -- Remove the main executable file and all tests *except* the current one
        remove_files("src/main.cpp")
        remove_files("src/test_*.cpp|src/" .. name .. ".cpp")

        add_tests("default")
end