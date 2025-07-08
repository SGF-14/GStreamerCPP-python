@echo off
echo Setting up Low Latency Video Streamer...
echo.

echo Checking prerequisites...

where python >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Python not found. Please install Python 3.9+
    pause
    exit /b 1
)

where node >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Node.js not found. Please install Node.js 18+
    pause
    exit /b 1
)

where gst-launch-1.0 >nul 2>nul
if %errorlevel% neq 0 (
    echo WARNING: GStreamer not found in PATH. Make sure it's installed.
)

echo.
echo Installing Python dependencies...
cd backend
pip install -r requirements.txt
cd ..

echo.
echo Installing frontend dependencies...
cd frontend
npm install
cd ..

echo.
echo Building C++ module...
call build_windows.bat

echo.
echo Setup complete!
echo.
echo To run the application:
echo 1. Start backend: cd backend && python .\dev_desktop.py
echo 2. Start frontend: cd frontend && npm run dev
echo 3. Open http://localhost:5173 in your browser
echo.
pause