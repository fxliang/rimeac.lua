@echo off
set base_dir=%CD%
set lua_dir=%base_dir%\lua-v5.4
set lua_version=54
set target=rimeac.lua

rem build x64 and x86 version of rimeac.lua
call :build x64 64
call :build x86 32 
goto end

rem param %1 should be 32 / 64, to make x86 and x64
:build
  call msvc-latest.bat %1

  if "%~2"=="32" (
    set libdir=%base_dir%\lib
    set distdir=%base_dir%\dist
  ) else (
    set libdir=%base_dir%\lib%2
    set distdir=%base_dir%\dist%2
  )

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
    rem link /DLL /out:lua%LV%.dll l*.obj
    lib /OUT:lua%LV%.lib l*.obj
    del *.o *.obj *.exp
    popd

  :skip_lua
    cl  /c /Zi /std:c++17 /EHsc /Iinclude /I%lua_dir% /I%base_dir%\LuaBridge3\Distribution main.cpp
    link /OUT:%target%.exe /PDB:%target%.pdb /LIBPATH:%libdir% /LIBPATH:%lua_dir% rime.lib main.obj 
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
