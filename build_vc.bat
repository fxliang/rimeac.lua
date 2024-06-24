@echo off
set base_dir=%CD%
set lua_dir=%base_dir%\lua-v5.4
set lua_version=54
set target=rimeac.lua

rem build x64 and x86 version of rimeac.lua
call :build x64
call :build x86
goto end

rem param %1 should be 32 / 64, to make x86 and x64
:build
  call msvc-latest.bat %1

  if "%~1"=="x86" (
    set distdir=%base_dir%\dist
  ) else (
    set distdir=%base_dir%\dist64
  )

  cmake -B%1 -GNinja . -DCMAKE_BUILD_TYPE=Release
  cmake --build %1
  cmake --install %1

  if exist %distdir%\usr (
    del /Q /S %distdir%\usr\*
    rmdir /Q /S %distdir%\usr
  )
  if exist %distdir%\log (
    rmdir /Q /S %distdir%\log
  )
  mkdir %distdir%\usr
  mkdir %distdir%\log

  exit /b

:end
  exit
