@echo off
setlocal enabledelayedexpansion

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ========================================================
    echo ERROR: This installer must be run as an Administrator^!
    echo ========================================================
    echo Please right-click install.bat and select "Run as administrator".
    echo.
    pause
    exit /b 1
)

set "DLL_NAME=AvifThumb.dll"
set "TARGET_DIR=%SystemRoot%\System32"
set "CLSID={B7A41C69-7788-4660-84E3-8E50C0A1B2C3}"
set "SOURCE_DLL=%~dp0%DLL_NAME%"

if not exist "%SOURCE_DLL%" (
    echo ========================================================
    echo ERROR: Cannot find %DLL_NAME%^!
    echo ========================================================
    echo Make sure install.bat is in the same folder as your DLL.
    echo Expected path: %SOURCE_DLL%
    echo.
    pause
    exit /b 1
)

echo [1/4] Clearing background COM processes...
taskkill /f /im dllhost.exe >nul 2>&1

echo [2/4] Copying %DLL_NAME% to %TARGET_DIR%...
copy /Y "%SOURCE_DLL%" "%TARGET_DIR%\%DLL_NAME%" >nul
if %errorLevel% neq 0 (
    echo ERROR: Failed to write to System32. File might be locked by the system.
    pause
    exit /b 1
)

echo [3/4] Registering COM values System-Wide for all users...

reg add "HKLM\Software\Classes\CLSID\%CLSID%" /ve /t REG_SZ /d "10-bit AVIF Thumbnail Provider" /f >nul

reg add "HKLM\Software\Classes\CLSID\%CLSID%\InprocServer32" /ve /t REG_SZ /d "%TARGET_DIR%\%DLL_NAME%" /f >nul
reg add "HKLM\Software\Classes\CLSID\%CLSID%\InprocServer32" /v "ThreadingModel" /t REG_SZ /d "Both" /f >nul

reg add "HKLM\Software\Classes\SystemFileAssociations\.avif\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /ve /t REG_SZ /d "%CLSID%" /f >nul

echo [4/4] Restarting Windows Explorer...
taskkill /f /im explorer.exe >nul 2>&1
timeout /t 1 /nobreak >nul
start explorer.exe

echo.
echo ========================================================
echo SUCCESS: Fastest AVIF Thumbnail Provider is installed globally^! Enjoy ^>^/^<
echo ========================================================
echo.
pause
exit /b 0