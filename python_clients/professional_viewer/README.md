# ESP32 Camera Viewer Pro

Professional PyQt6 camera viewer application with advanced capture features for ESP32 camera systems.

## Features

### 🎥 Live Stream Display
- Real-time MJPEG stream viewing
- FPS counter overlay
- Resolution display
- Smooth frame interpolation

### 📷 Capture Modes

#### Single Shot
- Click "Capture" button to save one frame
- Timestamped filename: `capture_YYYYMMDD_HHMMSS_fff.jpg`

#### Burst Mode
- **Long press** "Burst" button to activate
- Circular charging animation (blue → green → yellow)
- Frame counter display during capture
- Configurable capture rate (1-30 fps)
- Session-based filenames: `burst_SESSION_0001.jpg`

#### Video Recording
- Click "Record" to start/stop
- Choose filename and location
- Codec selection (MJPEG, H264, XVID)
- Recording duration timer
- Configurable FPS

### ⚙️ Settings
- Stream URL configuration
- Basic authentication support
- Save directory selection
- Burst capture rate (1-30 fps)
- JPEG quality (10-100)
- Video codec and FPS
- Settings persist across sessions

### 🎨 Modern UI
- Dark theme with cyan accents
- Clean, professional interface
- Status indicators
- Real-time statistics
- Responsive layout

## Installation

```bash
cd professional_viewer
pip install -r requirements.txt
```

## Usage

### Basic Usage

```bash
python camera_viewer_pro.py
```

The application will:
1. Load configuration from `config.json` (created on first run)
2. Connect to the default stream URL
3. Display live video feed

### Configuration

Edit `config.json` or use the Settings dialog (⚙️ button):

```json
{
  "stream": {
    "url": "http://192.168.2.100/stream",
    "username": "admin",
    "password": "",
    "auto_reconnect": true
  },
  "capture": {
    "save_directory": "./captures",
    "burst_fps": 5,
    "photo_quality": 95
  },
  "video": {
    "codec": "MJPEG",
    "fps": 20
  }
}
```

### Keyboard Shortcuts

*Note: Keyboard shortcuts can be added in future versions*

- `Space` - Single capture
- `B` - Toggle burst mode
- `V` - Start/stop recording
- `S` - Open settings
- `Q` - Quit

## Architecture

### Modular Design

```
professional_viewer/
├── camera_viewer_pro.py      # Main application
├── config.json                # Configuration file
├── requirements.txt           # Dependencies
├── widgets/
│   ├── stream_widget.py      # Live preview with FPS overlay
│   ├── burst_button.py       # Custom button with charging animation
│   └── settings_dialog.py    # Settings configuration dialog
├── workers/
│   ├── stream_worker.py      # Threaded MJPEG stream reader
│   ├── save_worker.py        # Background frame saving
│   └── video_worker.py       # Video encoding worker
├── utils/
│   ├── config_manager.py     # Configuration management
│   └── file_manager.py       # Smart filename generation
└── styles/
    └── dark_theme.qss        # Dark theme stylesheet
```

### Key Components

**StreamWidget**: Custom QLabel that displays video frames with FPS overlay

**BurstButton**: QPushButton with long-press detection and circular progress animation

**StreamWorker**: QThread that reads MJPEG stream and emits frames

**SaveWorker**: Background thread with queue for non-blocking file saves

**VideoWorker**: Handles video encoding with OpenCV VideoWriter

**ConfigManager**: JSON-based configuration with dot notation access

**FileManager**: Generates smart filenames with timestamps and session IDs

## How It Works

### Stream Reading
1. `StreamWorker` connects to MJPEG stream using `mjpeg_stream.py` helper
2. Frames are decoded and emitted via Qt signals
3. `StreamWidget` receives frames and displays with scaling
4. FPS is calculated from frame timestamps

### Burst Capture
1. User presses and holds "Burst" button
2. After 200ms threshold, burst mode activates
3. Timer captures frames at configured rate (default 5 fps)
4. Circular progress animation shows charge status
5. Frame counter updates in real-time
6. Frames are queued for background saving
7. Session ID ensures sequential filenames
8. Release button to stop

### Video Recording
1. User clicks "Record" and selects filename
2. `VideoWorker` initializes OpenCV VideoWriter
3. Every frame from stream is added to video
4. Recording indicator shows red status
5. Click "Stop" to finalize and close file

### Background Saving
1. All save operations use `SaveWorker` queue
2. Frames are copied before queueing (non-blocking)
3. Worker thread processes queue in background
4. JPEG quality applied during save
5. Queue size limited to prevent memory issues

## Burst Mode Animation

The burst button features a custom charging animation:

```
Normal State:          Charging (0.5s):       Charged (1.0s):
  [⏺ Burst]            [⏺ Burst]              [⏺ Burst]
                       ◐ 3 frames             ● 12 frames
                       (blue ring)            (green ring)
```

Color transitions:
- **0-50%**: Blue (charging)
- **50-80%**: Green (optimal)
- **80-100%**: Yellow (maximum)

## Requirements

- Python 3.8+
- PyQt6
- OpenCV (cv2)
- NumPy
- Requests (for authentication)

## Tested With

- ESP32-S3 + OV2640 camera
- MJPEG stream format
- HTTP basic authentication
- Various resolutions (QVGA to UXGA)

## Troubleshooting

### Stream Won't Connect

Check:
- ESP32 IP address is correct
- Stream URL ends with `/stream`
- Username/password if authentication enabled
- Firewall settings allow connection

### Burst Mode Not Working

- Must **hold** button (not click)
- Hold for at least 200ms
- Release button to stop capture
- Check save directory is writable

### Video Recording Issues

- Ensure codec is supported by your system
- MJPEG is most compatible
- H264 may require additional codecs
- Check disk space for large recordings

### Poor Performance

- Lower burst FPS in settings
- Reduce JPEG quality
- Use lower resolution stream
- Close other applications

## Future Enhancements

- [ ] Keyboard shortcuts
- [ ] Snapshot preview thumbnails
- [ ] Motion detection overlay
- [ ] Time-lapse mode
- [ ] Cloud upload integration
- [ ] Multi-camera support
- [ ] Recording scheduler
- [ ] Export to GIF

## License

MIT License - Use freely in your projects

## Credits

Based on the ESP32 camera viewer architecture and inspired by professional camera control software.

---

**Built with ❤️ for the ESP32 community**
