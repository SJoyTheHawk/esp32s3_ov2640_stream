@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if %ERRORLEVEL% EQU 0 goto run_with_py

where python >nul 2>nul
if %ERRORLEVEL% EQU 0 goto run_with_python

echo Python 3 was not found.
echo Install Python 3, then run: py -3 -m pip install -r requirements.txt
set "VIEWER_STATUS=127"
goto viewer_failed

:run_with_py
py -3 "%~dp0camera_viewer_pro.py"
goto viewer_finished

:run_with_python
python "%~dp0camera_viewer_pro.py"

:viewer_finished
set "VIEWER_STATUS=%ERRORLEVEL%"

if not "%VIEWER_STATUS%"=="0" goto viewer_failed
exit /b 0

:viewer_failed
echo.
echo Professional Viewer stopped with error code %VIEWER_STATUS%.
echo Make sure Python 3 and the required packages are installed:
echo   py -3 -m pip install -r "%~dp0requirements.txt"
echo.
pause
exit /b %VIEWER_STATUS%
