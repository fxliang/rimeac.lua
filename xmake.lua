set_languages("cxx17")
target("rimeac.lua")
  set_kind("binary")
  add_files("main.cpp", "lua-v5.4/*.c")
  add_includedirs("include", "./lua-v5.4", "./LuaBridge3/Distribution")
  remove_files("lua-v5.4/lua.c", "./lua-v5.4/onelua.c")
  set_symbols("debug")
  if is_plat("windows") then
    if is_arch("x64") then
      add_linkdirs("lib64")
    else
      add_linkdirs("lib")
    end
  end
  add_links("rime")

  on_install(function(target)
    if is_plat("windows") and has_config("vs") then
      local function install_files(target, files, dist_path)
        local function cp_file(file, src, dest)
          os.cp(path.join(src, file), dest)
          print("installed " .. path.join(dest, file))
        end
        cp_file(target:filename(), target:targetdir(), dist_path)
        cp_file("rimeac.lua.pdb", target:targetdir(), dist_path)
        for k, f in ipairs(files) do
          cp_file(f, "$(projectdir)", dist_path)
        end
      end
      if is_arch("x64") then
        local dist_path = path.join("$(projectdir)", "dist64")
        os.rm(path.join(dist_path, "log"))
        os.rm(path.join(dist_path, "usr"))
        install_files(target, {"script.lua", "README.md", "LICENSE.txt", "lib64/rime.dll", "shared", "usr"}, dist_path)
      else
        local dist_path = path.join("$(projectdir)", "dist")
        os.rm(path.join(dist_path, "log"))
        os.rm(path.join(dist_path, "usr"))
        install_files(target, {"script.lua", "README.md", "LICENSE.txt", "lib/rime.dll", "shared", "usr"}, dist_path)
      end
    elseif is_plat("linux") then
      os.cp(target:targetfile(), "$(projectdir)")
    end
  end)
