@echo off
setlocal

:: Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: Must be run as Administrator!
    pause
    exit /b 1
)

set "DLL_NAME=PNGThumb.dll"
set "TARGET_PATH=%SystemRoot%\System32\%DLL_NAME%"
set "CLSID={1A2B3C4D-5E6F-7A8B-9C0D-1E2F3A4B5C6D}"

echo [1/3] Killing COM hosts to release the DLL...
taskkill /f /im dllhost.exe >nul 2>&1

echo [2/3] Removing Registry keys...
:: Remove association keys first
reg delete "HKLM\Software\Classes\SystemFileAssociations\.png\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /f >nul 2>&1
reg delete "HKCR\.png\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /f >nul 2>&1
reg delete "HKCR\pngfile\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /f >nul 2>&1

:: Remove the CLSID key (this automatically removes InprocServer32 subkey)
reg delete "HKLM\Software\Classes\CLSID\%CLSID%" /f >nul 2>&1

echo [3/3] Deleting DLL...
if exist "%TARGET_PATH%" (
    del /f /q "%TARGET_PATH%"
) else (
    echo DLL not found in System32, skipping deletion.
)

echo.
echo Restarting Explorer...
taskkill /f /im explorer.exe >nul 2>&1
taskkill /f /im dllhost.exe >nul 2>&1
timeout /t 2 >nul
start explorer.exe

echo.
echo ========================================================
echo SUCCESS: Thumbnail provider uninstalled.
echo ========================================================
pause