@echo off
set base_dir=%CD%

rem build x64 and x86 version of rimeac.lua
xmake --version >nul 2>&1
if not errorlevel 1 (
  goto xmake
)
call :build x64
call :build x86
goto end

:xmake
  if not defined VSCMD_ARG_TGT_ARCH (
    call msvc-latest.bat x64
  )
  echo VCINSTALLDIR defined
  xmake f -a x64
  if not errorlevel 0 goto error
  xmake
  if not errorlevel 0 goto error
  xmake install
  if not errorlevel 0 goto error
  xmake f -a x86
  if not errorlevel 0 goto error
  xmake
  if not errorlevel 0 goto error
  xmake install
  if not errorlevel 0 goto error
  goto end

rem param %1 should be 32 / 64, to make x86 and x64
:build
  if not "%VSCMD_ARG_TGT_ARCH%"=="%1" (call msvc-latest.bat %1)

  if "%~1"=="x86" (
    set distdir=%base_dir%\dist
  ) else (
    set distdir=%base_dir%\dist64
  )

  cmake -B%1 -GNinja . -DCMAKE_BUILD_TYPE=Release
  if errorlevel 1 goto error
  cmake --build %1
  if errorlevel 1 goto error
  cmake --install %1
  if errorlevel 1 goto error

  if exist %distdir%\usr (
    del /Q /S %distdir%\usr\*
    rmdir /Q /S %distdir%\usr
  )
  if exist %distdir%\log (
    rmdir /Q /S %distdir%\log
  )
  mkdir %distdir%\usr

  exit /b

:end
  exit /b
:error
  echo error building targets
  pause
  exit /b
