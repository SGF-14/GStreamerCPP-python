@echo off
echo Building C++ GStreamer module for Windows...

cd streaming

if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

echo Building...
cmake --build . --config Release

echo Copying module to backend...
copy /Y Release\gst_streamer.pyd ..\..\backend\

cd ..\..

echo Build complete!
echo.
echo Make sure you have installed:
echo - GStreamer 1.22.12 or compatible (MSVC x64)
echo - Python development headers
echo - Visual Studio 2022 with C++ support
echo.
pause