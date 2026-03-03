@echo off
REM ============================================================
REM build.bat - Build helper for P4DevKitHome
REM
REM This script sets up the ESP-IDF environment and runs the
REM build so you don't have to remember all the paths.
REM
REM Usage:
REM   build.bat              - Just build
REM   build.bat flash COM5   - Build, flash, and monitor on COM5
REM   build.bat monitor COM5 - Just open serial monitor on COM5
REM   build.bat clean        - Full clean build
REM   build.bat menuconfig   - Open sdkconfig editor
REM ============================================================

REM --- ESP-IDF Environment Paths ---
set IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.5.1
set IDF_TOOLS_PATH=C:\Espressif
set IDF_PYTHON_ENV_PATH=C:\Espressif\python_env\idf5.5_py3.11_env

REM Clear MSYSTEM to avoid Git Bash/MSYS detection issues
set MSYSTEM=

REM Activate ESP-IDF environment (adds tools to PATH, etc.)
call "%IDF_PATH%\export.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to activate ESP-IDF environment.
    echo Make sure ESP-IDF v5.5.1 is installed at %IDF_PATH%
    exit /b 1
)

REM Navigate to firmware directory (where this script lives)
cd /d "%~dp0"

REM --- Handle command-line arguments ---
if "%1"=="" goto :build
if "%1"=="flash" goto :flash
if "%1"=="monitor" goto :monitor
if "%1"=="clean" goto :clean
if "%1"=="menuconfig" goto :menuconfig
if "%1"=="set-target" goto :settarget

echo Unknown command: %1
echo.
echo Usage:
echo   build.bat              - Build the project
echo   build.bat flash COM5   - Build, flash, and monitor
echo   build.bat monitor COM5 - Open serial monitor
echo   build.bat clean        - Full clean and rebuild
echo   build.bat menuconfig   - Open sdkconfig editor
echo   build.bat set-target   - Set target to esp32p4
exit /b 1

:build
echo === Building P4DevKitHome ===
idf.py build
goto :end

:flash
if "%2"=="" (
    echo ERROR: Please specify COM port. Example: build.bat flash COM5
    exit /b 1
)
echo === Building and Flashing on %2 ===
idf.py -p %2 flash monitor
goto :end

:monitor
if "%2"=="" (
    echo ERROR: Please specify COM port. Example: build.bat monitor COM5
    exit /b 1
)
echo === Opening Serial Monitor on %2 ===
idf.py -p %2 monitor
goto :end

:clean
echo === Full Clean Build ===
idf.py fullclean
idf.py set-target esp32p4
idf.py build
goto :end

:menuconfig
echo === Opening menuconfig ===
idf.py menuconfig
goto :end

:settarget
echo === Setting target to esp32p4 ===
idf.py set-target esp32p4
goto :end

:end
