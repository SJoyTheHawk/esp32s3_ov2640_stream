@echo off
setlocal
set "PACKAGE_DIR=%~dp0"
set "VIEWER_DIR=%PACKAGE_DIR%professional_viewer"
cd /d "%VIEWER_DIR%"

where py >nul 2>nul
if %ERRORLEVEL% EQU 0 goto run_with_py
where python >nul 2>nul
if %ERRORLEVEL% EQU 0 goto run_with_python

echo Python 3 was not found. Install Python 3.10 or newer, then run:
echo   "%PACKAGE_DIR%Install Dependencies.bat"
pause
exit /b 127

:run_with_py
py -3 "%VIEWER_DIR%\camera_viewer_pro.py"
set "STATUS=%ERRORLEVEL%"
goto finished

:run_with_python
python "%VIEWER_DIR%\camera_viewer_pro.py"
set "STATUS=%ERRORLEVEL%"

:finished
if not "%STATUS%"=="0" (
  echo.
  echo Professional Viewer stopped with error code %STATUS%.
  pause
)
exit /b %STATUS%
