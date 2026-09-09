@echo off
setlocal
set "PACKAGE_DIR=%~dp0"
cd /d "%PACKAGE_DIR%"

where py >nul 2>nul
if %ERRORLEVEL% EQU 0 goto install_with_py
where python >nul 2>nul
if %ERRORLEVEL% EQU 0 goto install_with_python

echo Python 3 was not found. Install Python 3.10 or newer first.
pause
exit /b 127

:install_with_py
py -3 -m pip install -r "%PACKAGE_DIR%requirements.txt"
set "STATUS=%ERRORLEVEL%"
goto finished

:install_with_python
python -m pip install -r "%PACKAGE_DIR%requirements.txt"
set "STATUS=%ERRORLEVEL%"

:finished
if "%STATUS%"=="0" echo Dependencies installed successfully.
if not "%STATUS%"=="0" echo Dependency installation failed with code %STATUS%.
pause
exit /b %STATUS%
