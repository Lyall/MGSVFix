set_project("MGSVFix")
add_rules("mode.debug", "mode.release")
set_languages("cxxlatest", "clatest")
set_optimize("faster")

  target("MGSVFix")
    set_kind("shared")
    add_files("src/**.cpp", "external/safetyhook/safetyhook.cpp", "external/safetyhook/Zydis.c")
    add_syslinks("user32")
    add_includedirs("external/spdlog/include", "external/inipp", "external/safetyhook")
    set_prefixname("")
    set_extension(".asi")

  -- Set platform specific toolchain
  if is_plat("windows") then
    set_toolchains("msvc")
    add_cxflags("/utf-8")
    if is_mode("release") then
      add_cxflags("/MT")
    elseif is_mode("debug") then
      add_cxflags("/MTd")
    end
  end
