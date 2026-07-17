@echo off
call devcmd

cl.exe /I . /EHsc /O2 /Oi /Gw /Gy /GL /MD /LD dllmain.cpp ThumbnailProvider.cpp /Fe:AvifThumb.dll /link /LTCG /DEF:exports.def dav1d.lib avif.lib yuv.lib Ole32.lib Gdi32.lib User32.lib Shlwapi.lib

del *.obj
del *.exp
del AvifThumb.lib