## Fastest AVIF thumbnail generator for Windows Explorer

### Build
- Requirements: Python, Meson (```pip install meson```), CMake, Ninja, NASM, Portable Build Tools (devcmd) (You can get most at Sputchik/pdi)
> update.bat


### FAQYOU
- Why? - over 3x faster than default, support for 8/10/12 bit-depth images (main reason)

### One-Liner
> curl -L -# "https://github.com/Sputchik/avif_winexplorer/raw/refs/heads/main/AvifThumb.dll" -o C:\Windows\System32\AvifThumb.dll && taskkill /f /im dllhost.exe >nul 2>&1 && reg add "HKLM\Software\Classes\CLSID\{B7A41C69-7788-4660-84E3-8E50C0A1B2C3}" /ve /t REG_SZ /d "AVIF Thumbnail Provider" /f >nul && reg add "HKLM\Software\Classes\CLSID\{B7A41C69-7788-4660-84E3-8E50C0A1B2C3}\InprocServer32" /ve /t REG_SZ /d "C:\Windows\System32\AvifThumb.dll" /f >nul && reg add "HKLM\Software\Classes\CLSID\{B7A41C69-7788-4660-84E3-8E50C0A1B2C3}\InprocServer32" /v "ThreadingModel" /t REG_SZ /d "Both" /f >nul && reg add "HKLM\Software\Classes\SystemFileAssociations\.avif\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /ve /t REG_SZ /d "{B7A41C69-7788-4660-84E3-8E50C0A1B2C3}" /f >nul && taskkill /f /im explorer.exe >nul 2>&1 && timeout /t 1 /nobreak >nul && start explorer.exe