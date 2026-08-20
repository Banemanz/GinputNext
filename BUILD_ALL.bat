@echo off
setlocal EnableExtensions
set "ROOT=%~dp0"
set "NOPAUSE=0"
if /I "%~1"=="--no-pause" set "NOPAUSE=1"

if "%PLUGIN_SDK_DIR%"=="" (
  echo ERROR: PLUGIN_SDK_DIR is not set.
  set "RC=1"
  goto :done
)

for %%L in (plugin_iii.lib plugin_vc.lib plugin.lib) do (
  if not exist "%PLUGIN_SDK_DIR%\output\lib\%%L" (
    echo ERROR: Missing "%PLUGIN_SDK_DIR%\output\lib\%%L"
    echo Build the required Plugin-SDK game library only; this project does not build plugin.sln.
    set "RC=1"
    goto :done
  )
)

echo.
echo ============================================================
echo Preparing shared x86 SDL2/controller database dependencies...
echo ============================================================
call "%ROOT%PREPARE_DEPS.bat" --no-pause
if errorlevel 1 (
  set "RC=1"
  goto :done
)

echo.
echo ============================================================
echo [1/3] Building GTA III...
echo ============================================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\BUILD_PROJECT.ps1" -Project GInputNext_GTA3 -SkipDependencyFetch
if errorlevel 1 (
  set "RC=1"
  goto :done
)

if not exist "%ROOT%dist\GTA3\GInputNext\GInputNext.asi" (
  echo ERROR: GTA III build returned success but ASI is missing.
  set "RC=1"
  goto :done
)

echo.
echo ============================================================
echo [2/3] Building Vice City...
echo ============================================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\BUILD_PROJECT.ps1" -Project GInputNext_GTAVC -SkipDependencyFetch
if errorlevel 1 (
  set "RC=1"
  goto :done
)

if not exist "%ROOT%dist\GTAVC\GInputNext\GInputNext.asi" (
  echo ERROR: Vice City build returned success but ASI is missing.
  set "RC=1"
  goto :done
)

echo.
echo ============================================================
echo [3/3] Building San Andreas...
echo ============================================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\BUILD_PROJECT.ps1" -Project GInputNext_GTASA -SkipDependencyFetch
if errorlevel 1 (
  set "RC=1"
  goto :done
)

if not exist "%ROOT%dist\GTASA\GInputNext\GInputNext.asi" (
  echo ERROR: San Andreas build returned success but ASI is missing.
  set "RC=1"
  goto :done
)

set "RC=0"
echo.
echo ============================================================
echo GInputNext build complete: GTA III + VC + SA
echo GTA III: "%ROOT%dist\GTA3\GInputNext"
echo VC:      "%ROOT%dist\GTAVC\GInputNext"
echo SA:      "%ROOT%dist\GTASA\GInputNext"
echo ============================================================

goto :done

:done
echo.
if not "%RC%"=="0" (
  echo BUILD_ALL FAILED.
  echo If failure happened before [1/3], it was dependency preparation.
  echo Otherwise check "%ROOT%build_logs" for the per-game build log.
)
if "%NOPAUSE%"=="0" pause
exit /b %RC%
