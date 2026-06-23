@echo off
call devcmd

set "DEPS=%~dp0deps"
if not exist "%DEPS%" mkdir "%DEPS%"
set "OPT_FLAGS=/GL /arch:AVX2 /Oi /O2 /Ob2 /Gw /Gy /fp:fast"
@REM goto :sok

if not exist libyuv ( git clone --single-branch https://chromium.googlesource.com/libyuv/libyuv --depth 1 )
cd libyuv
git pull
cd ..

cmake -G Ninja -S libyuv -B libyuv/build ^
    -DCMAKE_C_FLAGS="%OPT_FLAGS%" ^
    -DCMAKE_CXX_FLAGS="%OPT_FLAGS%" ^
    -DCMAKE_INSTALL_PREFIX="%DEPS%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DENABLE_SHARED=OFF ^
    -DENABLE_STATIC=ON ^
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF ^
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON

cmake --build libyuv/build --config Release --target yuv --parallel %NUMBER_OF_PROCESSORS% --target install


if not exist ("dav1d") (
    git clone https://code.videolan.org/videolan/dav1d.git --depth 1
    cd dav1d
) else (
    cd dav1d
    git pull
)
if exist ("build") (
    rmdir /s /q build
)

mkdir build && cd build
meson setup .. --buildtype release --default-library static -Db_lto=true -Db_ndebug=true --wipe
ninja
copy /Y src\libdav1d.a ..\..\
cd ..\..

if exist ("dav1d.lib") del dav1d.lib
ren libdav1d.a dav1d.lib
copy dav1d.lib "%DEPS%\lib\dav1d.lib" /Y

:sok
if not exist libavif (
    git clone https://github.com/AOMediaCodec/libavif.git --recursive
    cd libavif
) else (
    cd libavif
    git pull
    git submodule update --init --recursive
)

if exist ("build") ( rmdir /s /q build )
mkdir build
cd build

cmake .. -G Ninja ^
  -DCMAKE_C_FLAGS="%OPT_FLAGS%" ^
  -DCMAKE_CXX_FLAGS="%OPT_FLAGS%" ^
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="%DEPS%" ^
  -DCMAKE_INSTALL_PREFIX="%DEPS%" ^
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DAVIF_BUILD_TESTS=OFF ^
  -DAVIF_BUILD_APPS=OFF ^
  -DAVIF_BUILD_EXAMPLES=OFF ^
  -DAVIF_LIBYUV=SYSTEM ^
  -DAVIF_CODEC_AOM=OFF ^
  -DAVIF_CODEC_DAV1D=SYSTEM ^
  -DDAV1D_INCLUDE_DIR="../../dav1d/include/" ^
  -DDAV1D_LIBRARY="%DEPS%\lib\dav1d.lib" ^
  -DAVIF_ZLIBPNG=OFF ^
  -DAVIF_JPEG=OFF

cmake --build "." --config Release --target install --parallel %NUMBER_OF_PROCESSORS%

cd "..\.."
if not exist ("avif") (
    mkdir "avif"
)

xcopy /Y /I "libavif\include\avif\" "avif\"
xcopy /Y /I "%DEPS%\lib\avif.lib" ".\"
xcopy /Y /I "%DEPS%\lib\yuv.lib" ".\"
call build.bat