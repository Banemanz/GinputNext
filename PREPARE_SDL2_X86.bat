@echo off
setlocal EnableExtensions
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\FETCH_SDL2.ps1" -Force
set "RC=%ERRORLEVEL%"
echo.
if "%RC%"=="0" (
  echo SDL2 x86 preparation completed successfully.
) else (
  echo SDL2 x86 preparation FAILED with exit code %RC%.
)
pause
exit /b %RC%
