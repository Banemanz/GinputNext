@echo off
setlocal EnableExtensions
set "NOPAUSE=0"
if /I "%~1"=="--no-pause" set "NOPAUSE=1"

echo Preparing official SDL2 2.32.10 x86...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\FETCH_SDL2.ps1"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" goto :fail

echo Preparing controller mapping database...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\FETCH_CONTROLLER_DB.ps1"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" goto :fail

echo.
echo Dependencies ready.
if "%NOPAUSE%"=="0" pause
exit /b 0

:fail
echo.
echo Dependency preparation FAILED with exit code %RC%.
if "%NOPAUSE%"=="0" pause
exit /b %RC%
