@echo off
setlocal EnableExtensions
set "NOPAUSE=0"
if /I "%~1"=="--no-pause" set "NOPAUSE=1"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\BUILD_PROJECT.ps1" -Project GInputNext_GTASA
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
  echo San Andreas build completed successfully.
) else (
  echo San Andreas build FAILED with exit code %RC%.
  echo See "%~dp0build_logs\GInputNext_GTASA.log"
)

if "%NOPAUSE%"=="0" pause
exit /b %RC%
