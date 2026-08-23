@echo off
setlocal
set "ROOT=%~dp0"

where powershell >nul 2>nul
if %errorlevel%==0 (
  powershell -ExecutionPolicy Bypass -File "%ROOT%RUN_MONOPOLY.ps1"
  exit /b %errorlevel%
)

where pwsh >nul 2>nul
if %errorlevel%==0 (
  pwsh -ExecutionPolicy Bypass -File "%ROOT%RUN_MONOPOLY.ps1"
  exit /b %errorlevel%
)

echo PowerShell not found.
exit /b 1
