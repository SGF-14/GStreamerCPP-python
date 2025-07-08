# Low Latency RTSP Video Streamer

A high-performance video streaming application that uses C++ with GStreamer for low-latency RTSP streaming, similar to QGroundControl's implementation. Features a Svelte frontend and Python backend with WebRTC for browser display.

## Architecture

- **C++ GStreamer Module**: Handles RTSP stream capture and H.264/H.265 decoding with optimizations for low latency
- **Python Backend**: Bridges between C++ module and frontend using WebRTC
- **Svelte Frontend**: Modern web interface with real-time video display and statistics

## Prerequisites

### Windows Setup

1. **Install GStreamer 1.22.12 MSVC x64**
   - Download from: https://gstreamer.freedesktop.org/data/pkg/windows/1.22.12/msvc/gstreamer-1.0-msvc-x86_64-1.22.12.msi
   - Choose "Complete" installation (not "Typical")
   - Add to PATH: `C:\gstreamer\1.0\msvc_x86_64\bin`

2. **Install Visual Studio 2022**
   - Include "Desktop development with C++" workload
   - Include CMake tools

3. **Install Python 3.9+**
   - Download from python.org
   - Add to PATH during installation

4. **Install Node.js 18+**
   - Download from nodejs.org

## Installation

### 1. Clone the repository
```bash
git clone <repository>
cd video-streamer
```

### 2. Build C++ Module
```bash
# Install Python development package
pip install pybind11

# Build the C++ module
build_windows.bat
```

### 3. Setup Backend
```bash
cd backend
pip install -r requirements.txt
```

### 4. Setup Frontend
```bash
cd frontend
npm install
```

## Running the Application

### 1. Start the Backend
```bash
cd backend
python .\dev_desktop.py
```
The backend will start on `http://localhost:8000`

### 2. Start the Frontend
```bash
cd frontend
npm run dev
```
The frontend will start on `http://localhost:5173`

### 3. Using the Application
1. Open `http://localhost:5173` in your browser
2. Enter your RTSP URL (e.g., `rtsp://192.168.1.100:554/stream`)
3. Click "Start Stream"
4. Monitor real-time statistics (FPS, bitrate, latency)

## Low Latency Optimizations

The application implements several optimizations similar to QGroundControl:

### GStreamer Pipeline
- **Low Latency Mode**: Removes jitter buffer for minimal delay
- **Hardware Acceleration**: Uses available hardware decoders
- **Zero-Copy**: Minimizes memory copies in the pipeline
- **Tuned Buffers**: Single frame buffering in low latency mode

### Key Components:
```
RTSP Source → H.264 Depay → Parse → Decode → Convert → WebRTC
```

### Latency Reduction Techniques:
1. **No Jitter Buffer**: Direct pipeline without buffering
2. **TCP Transport**: More reliable than UDP for RTSP
3. **Fast Decoding**: Multi-threaded decoder with 4 threads
4. **Zero B-Frames**: Encoder configured without B-frames
5. **Minimal Lookahead**: No RC lookahead for encoding

## Configuration Options

### Low Latency Mode
- Reduces latency to ~100-200ms
- May cause frame drops on poor networks
- Disables synchronization

### Standard Mode  
- Latency ~300-500ms
- Better stability
- Includes jitter buffer

### Bitrate Control
- Adjustable from 500 kbps to 50 Mbps
- Dynamic adaptation based on network

### Resolution Support
- 480p, 720p, 1080p, 4K
- Hardware scaling for performance

## Troubleshooting

### No Video Display
1. Check GStreamer installation:
   ```bash
   gst-launch-1.0 --version
   ```
2. Verify RTSP URL is accessible
3. Check browser console for WebRTC errors

### High Latency
1. Enable "Low Latency Mode" in settings
2. Reduce resolution/bitrate
3. Check network conditions

### Build Errors
1. Ensure all prerequisites are installed
2. Check GStreamer environment variables
3. Verify Python development headers

## Performance Tips

1. **Use Hardware Encoding**: GPU acceleration significantly reduces latency
2. **Wired Connection**: Ethernet provides more stable streaming than WiFi
3. **Local Network**: Best performance on LAN with minimal hops
4. **Modern Browser**: Chrome/Edge provide best WebRTC performance

## Architecture Details

### C++ Module (`gst_rtsp_streamer`)
- Manages GStreamer pipeline lifecycle
- Handles RTSP connection and reconnection
- Provides Python bindings via pybind11
- Implements frame callbacks for data transfer

### Python Backend (`dev_desktop.py`)
- WebRTC signaling server
- REST API for stream control
- Bridges C++ frames to WebRTC
- Configuration management

### Frontend (`VideoPlayer.svelte`)
- WebRTC peer connection management
- Real-time statistics monitoring
- Responsive video display
- Stream control interface

## License
MIT