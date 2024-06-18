rem @echo off
set base_dir=%CD%
set lua_dir=%base_dir%\lua-v5.4
set lua_version=54
set target=rimeac.lua

if not exist include\LuaBridge\LuaBridge.h (
  xcopy /E /I LuaBridge3\Distribution\LuaBridge include\LuaBridge
)

call :build x64 64
call :build x86 32 
goto end

:build
  call msvc-latest.bat %1

  if "%~2"=="32" (
    set libdir=%base_dir%\lib
    set distdir=%base_dir%\dist
  ) else (
    set libdir=%base_dir%\lib%2
    set distdir=%base_dir%\dist%2
  )

  if exist %libdir%\lua54.lib goto skip_lua
  :build_lua
    pushd %lua_dir%
    set LV=%lua_version%
    if exist *.o del *.o
    if exist *.obj del *.obj
    if exist *.exe del *.exe
    if exist *.dll del *.dll
    if exist *.lib del *.lib
    if exist *.exp del *.exp
    cl /O2 /W3 /c /DLUA_BUILD_AS_DLL l*.c
    del lua.obj luac.obj
    link /DLL /out:lua%LV%.dll l*.obj
    del *.o *.obj *.exp
    popd
    copy /y %lua_dir%\lua.h %base_dir%\include\
    copy /y %lua_dir%\luaconf.h %base_dir%\include\
    copy /y %lua_dir%\lualib.h %base_dir%\include\
    copy /y %lua_dir%\lauxlib.h %base_dir%\include\

    copy /y %lua_dir%\lua54.dll %libdir%
    copy /y %lua_dir%\lua54.lib %libdir%

  :skip_lua
    cl  /c /Zi /std:c++17 /EHsc /Iinclude main.cpp
    link /OUT:%target%.exe /PDB:%target%.pdb /LIBPATH:%libdir% rime.lib main.obj 
    move vc*.pdb %distdir%\%target%.pdb
    move %target%.exe %distdir%\
    copy /y script.lua %distdir%\
    copy /y README.md %distdir%\
    copy /y LICENSE.txt %distdir%\
    del main.obj
    if exist %distdir%\usr (
      del /Q /S %distdir%\usr\*
      rmdir /Q /S %distdir%\usr
    )
    if exist %distdir%\log (
      rmdir /Q /S %distdir%\log
    )
    if not exist %distdir%\shared ( xcopy /E /I shared %distdir%\shared )
    mkdir %distdir%\usr
    mkdir %distdir%\log

    exit /b

:end
  exit
