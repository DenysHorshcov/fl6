@echo off
if "%~1"=="" (
    echo Drag and drop .cpp file onto this .bat
    pause
    exit /b
)

REM %~1 - перший аргумент, шлях до файлу, який кинули на .bat
count_ref_lambdas.exe "%~1" -std=c++17

echo.
echo Done.
pause
