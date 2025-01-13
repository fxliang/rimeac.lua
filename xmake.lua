set_languages("cxx17")

if not is_plat("windows") and not is_plat('mingw') then
add_requires("lua")
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
  if is_plat("windows", "mingw") then
    if is_plat('windows') then
      add_shflags("/LARGEADDRESSAWARE")
      add_cxflags("/utf-8")
    else
      add_shflags('-static-libgcc -static-libstdc++ -static', {force=true})
    end
    add_linkdirs(is_arch("x64", "x86_64") and "lib64" or "lib")
    add_includedirs("include", "lua-v5.4")
    add_deps("lua54_lib")
  else
    add_packages('lua')
  end
  on_install(function(target)
    if is_plat("windows", "mingw") then
      local function cp_file(file, src, dest)
        os.trycp(path.join(src, file), dest)
        print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
      end
      local dist_path = path.join("$(projectdir)", is_arch("x64", "x86_64") and "dist64" or "dist")
      cp_file(target:filename(), target:targetdir(), dist_path)
    else
      os.trycp(path.join(target:targetdir(), target:filename()), "$(projectdir)")
    end
  end)

-- prepare lua.exe and lua54.dll for windows msvc and mingw build
if is_plat("windows", "mingw") then
  target("lua54_lib")
    set_kind("static")
    add_defines("MAKE_LIB")
    add_files("lua-v5.4/onelua.c")
    set_filename("lua54.lib")
    before_build(function(target)
      local target_dir = path.join(target:targetdir(), target:name())
      if not os.exists(target_dir) then
        os.mkdir(target_dir)
      end
      target:set("targetdir", target_dir)
      target:set("filename", "lua54.lib")
    end)
    on_install(function(target) return end)

  target("lua54")
    set_kind("shared")
    add_defines("MAKE_LIB", "LUA_BUILD_AS_DLL")
    add_files("lua-v5.4/onelua.c")
    add_rules("install_target", "add_shflags")

  target("lua")
    set_kind("binary")
    add_files("lua-v5.4/lua.c")
    add_deps("lua54", {config={shared = true}})
    add_rules("install_target", "add_shflags")

  rule("add_shflags")
    on_load(function(target)
    if is_plat("windows") then
      target:add("shflags", "/LARGEADDRESSAWARE", {force=true})
    else
      target:add("shflags", '-static-libgcc -static-libstdc++ -static', {force=true})
    end
    end)

  rule("install_target")
    on_install(function(target)
      local function cp_file(file, src, dest)
        os.trycp(path.join(src, file), dest)
        print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
      end
      local dist_path = is_arch("x64", "x86_64") and "dist64" or "dist"
      cp_file(target:filename(), target:targetdir(), dist_path)
    end)
end
-- prepare lua.exe and lua54.dll for windows msvc build end

-- target rimeac.lua.exe
-- lua static link
target("rimeac.lua")
  set_kind("binary")
  add_files("main.cpp")
  if is_plat("windows", "mingw") then
    add_linkdirs(is_arch("x64", "x86_64") and "lib64" or "lib")
    add_includedirs("include", "lua-v5.4")
    add_deps("lua54_lib")
    if is_plat('mingw') then
      add_ldflags('-static-libgcc -static-libstdc++ -static', {force=true})
    elseif is_plat('windows') then
      set_symbols("debug")
      add_cxflags("/utf-8")
      add_ldflags("/LARGEADDRESSAWARE")
    end
  else
    add_packages('lua')
  end
  add_links("rime")

  on_install(function(target)
    if is_plat("windows", "mingw") then
      local function install_files(target, files, dist_path)
        local function cp_file(file, src, dest)
          if os.exists(path.join(src, file)) then
            os.trycp(path.join(src, file), dest)
            print("installed " .. path.join(dest, path.filename(path.join(dest, file))))
          end
        end
        cp_file(target:filename(), target:targetdir(), dist_path)
        if is_plat('windows') then cp_file(target:basename()..".pdb", target:targetdir(), dist_path) end
        for _, f in ipairs(files) do cp_file(f, "$(projectdir)", dist_path) end
      end
      local dist_path = path.join("$(projectdir)", is_arch("x64", "x86_64") and "dist64" or "dist")
      local rimedll = is_arch("x64", "x86_64") and "lib64/rime.dll" or "lib/rime.dll"
      local rimepdb = is_arch("x64", "x86_64") and "lib64/rime.pdb" or "lib/rime.pdb"
      os.rm(path.join(dist_path, "log"))
      os.rm(path.join(dist_path, "usr"))
      install_files(target, {"script.lua", "script_ext.lua", "rime_api_console.lua", "README.md", "LICENSE.txt", rimedll, rimepdb, "shared", "usr"}, dist_path)
      os.rm(path.join(dist_path, "usr/.placeholder"))
    elseif is_plat("linux") then os.trycp(target:targetfile(), "$(projectdir)") end
  end)
