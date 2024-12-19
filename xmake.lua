set_languages("cxx17")

if not is_plat("windows") then
add_requires("lua")
add_requires("luabridge3")
add_defines("XMAKE_REPO")
end

-- target librimeac.dll for windows or librimeac.so for linux
-- lua static link
if is_plat("windows") then
target("librimeac")
else
target("rimeac")
end
  set_kind("shared")
  add_files("main.cpp")
  add_links("rime")
  add_defines("MODULE")
  if is_plat("windows") then
    add_shflags("/LARGEADDRESSAWARE")
    if is_arch("x64") then
      add_linkdirs("lib64")
    else
      add_linkdirs("lib")
    end
    add_includedirs("include", "lua-v5.4", "LuaBridge3/Distribution")
    add_deps("lua54_lib")
  else
    add_packages('lua', 'luabridge3')
  end
  on_install(function(target)
    if is_plat("windows") then
      local function cp_file(file, src, dest)
        os.cp(path.join(src, file), dest)
        print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
      end
      if is_arch("x64") then
        local dist_path = path.join("$(projectdir)", "dist64")
        cp_file("script_ext.lua", "$(projectdir)", dist_path)
        cp_file(target:filename(), target:targetdir(), dist_path)
      else
        local dist_path = path.join("$(projectdir)", "dist")
        cp_file("script_ext.lua", "$(projectdir)", dist_path)
        cp_file(target:filename(), target:targetdir(), dist_path)
      end
    else
      os.cp(path.join(target:targetdir(), target:filename()), "$(projectdir)")
    end
  end)

-- prepare lua.exe and lua54.dll for windows msvc build
if is_plat("windows") then
  target("lua54_lib")
    set_kind("static")
    add_files("lua-v5.4/*.c")
    remove_files("lua-v5.4/lua.c", "lua-v5.4/onelua.c")
    set_filename("lua54.lib")
    before_build(function(target)
      local target_dir = path.join(target:targetdir(), target:name())
      if not os.exists(target_dir) then
        os.mkdir(target_dir)
      end
      target:set("targetdir", target_dir)
      target:set("filename", "lua54.lib")
    end)

  target("lua54")
    set_kind("shared")
    add_files("lua-v5.4/*.c")
    remove_files("lua-v5.4/lua.c", "lua-v5.4/onelua.c")
    add_defines("LUA_BUILD_AS_DLL")
    if is_plat("windows") then
      add_shflags("/LARGEADDRESSAWARE", {force=true})
    end
    on_install(function(target)
      if is_plat("windows") then
        local function cp_file(file, src, dest)
          os.cp(path.join(src, file), dest)
          print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
        end
        if is_arch("x64") then
          local dist_path = path.join("$(projectdir)", "dist64")
          cp_file(target:filename(), target:targetdir(), dist_path)
        else
          local dist_path = path.join("$(projectdir)", "dist")
          cp_file(target:filename(), target:targetdir(), dist_path)
        end
      end
    end)
  target("lua")
    set_kind("binary")
    add_files("lua-v5.4/lua.c")
    add_deps("lua54", {config={shared = true}})
    if is_plat("windows") then
      add_ldflags("/LARGEADDRESSAWARE")
    end
    on_install(function(target)
      if is_plat("windows") then
        local function cp_file(file, src, dest)
          os.cp(path.join(src, file), dest)
          print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
        end
        if is_arch("x64") then
          local dist_path = path.join("$(projectdir)", "dist64")
          cp_file(target:filename(), target:targetdir(), dist_path)
        else
          local dist_path = path.join("$(projectdir)", "dist")
          cp_file(target:filename(), target:targetdir(), dist_path)
        end
      end
    end)
end
-- prepare lua.exe and lua54.dll for windows msvc build end

-- target rimeac.lua.exe
-- lua static link
target("rimeac.lua")
  set_kind("binary")
  add_files("main.cpp")
  set_symbols("debug")
  if is_plat("windows") then
    add_ldflags("/LARGEADDRESSAWARE")
    if is_arch("x64") then
      add_linkdirs("lib64")
    else
      add_linkdirs("lib")
    end
    add_includedirs("include", "lua-v5.4", "LuaBridge3/Distribution")
    add_deps("lua54_lib")
  else
    add_packages('lua', 'luabridge3')
  end

  add_links("rime")

  on_install(function(target)
    if is_plat("windows") and has_config("vs") then
      local function install_files(target, files, dist_path)
        local function cp_file(file, src, dest)
          os.cp(path.join(src, file), dest)
          print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
        end
        cp_file(target:filename(), target:targetdir(), dist_path)
        cp_file("rimeac.lua.pdb", target:targetdir(), dist_path)
        for _, f in ipairs(files) do
          cp_file(f, "$(projectdir)", dist_path)
        end
      end
      local dist_path = ''
      if is_arch("x64") then
        dist_path = path.join("$(projectdir)", "dist64")
        os.rm(path.join(dist_path, "log"))
        os.rm(path.join(dist_path, "usr"))
        install_files(target, {"script.lua", "script_ext.lua", "rime_api_console.lua", "README.md", "LICENSE.txt", "lib64/rime.dll", "shared", "usr"}, dist_path)
        os.rm(path.join(dist_path, "usr/.placeholder"))
      else
        dist_path = path.join("$(projectdir)", "dist")
        os.rm(path.join(dist_path, "log"))
        os.rm(path.join(dist_path, "usr"))
        install_files(target, {"script.lua", "script_ext.lua", "rime_api_console.lua", "README.md", "LICENSE.txt", "lib/rime.dll", "shared", "usr"}, dist_path)
        os.rm(path.join(dist_path, "usr/.placeholder"))
      end
    elseif is_plat("linux") then
      os.cp(target:targetfile(), "$(projectdir)")
    end
  end)
