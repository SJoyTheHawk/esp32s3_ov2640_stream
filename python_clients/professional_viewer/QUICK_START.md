# Quick Start Guide - Professional Camera Viewer

## Installation

```bash
cd /Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/python_clients/professional_viewer

# Install dependencies
pip install -r requirements.txt
```

## Launch

```bash
# Method 1: Direct
python3 camera_viewer_pro.py

# Method 2: Using the shell launcher
./run.sh

# Method 3: Run tests first
python3 test_components.py && python3 camera_viewer_pro.py
```

### Double-click launchers

- **macOS:** Double-click `Professional Viewer.command`. If macOS blocks it after
  downloading or copying, Control-click it, choose **Open**, then confirm **Open**.
- **Windows:** Double-click `Professional Viewer.bat`.

The launchers use Python installed on the computer. Install the dependencies once
before the first launch:

```bash
# macOS
python3 -m pip install -r requirements.txt

# Windows (Command Prompt)
py -3 -m pip install -r requirements.txt
```

## First Time Setup

1. **Configure Stream URL**
   - Click "⚙ Settings" button
   - Enter your ESP32 IP: `http://192.168.2.100/stream`
   - Add username/password if authentication enabled
   - Click "Save"

2. **Set Save Directory**
   - In Settings dialog
   - Click "Browse" next to Save Directory
   - Choose where to save captures
   - Click "Save"

## Basic Usage

### Single Capture
1. Wait for stream to connect (green indicator)
2. Click "📷 Capture" button
3. Photo saved to captures directory
4. Notification shows in status bar

### Burst Mode
1. **Press and HOLD** "⏺ Burst" button
2. Watch charging animation (blue → green → yellow)
3. Frame counter shows captured frames
4. **Release** button to stop
5. All frames saved with session ID

### Video Recording
1. Click "🎥 Record" button
2. Choose filename and location
3. Recording starts (button turns red)
4. Click "⏹ Stop" to end recording
5. Video saved to chosen location

## Hotkeys

- Click capture button: Single photo
- Hold burst button: Continuous capture
- Click record button: Toggle video recording

## Troubleshooting

### Stream won't connect
- Check ESP32 IP address in URL field
- Ensure ESP32 is powered on and connected to WiFi
- Verify network connectivity
- Check firewall settings

### Burst mode not working
- Must **hold** button (not just click)
- Hold for at least 200ms
- Watch for charging animation

### Video won't save
- Ensure sufficient disk space
- Try MJPEG codec (most compatible)
- Check save directory permissions

## Configuration

Edit `config.json` directly or use Settings dialog:

```json
{
  "stream": {
    "url": "http://192.168.2.100/stream",
    "username": "admin",
    "password": ""
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

## Tips

- **Better quality**: Increase `photo_quality` to 100
- **Faster burst**: Increase `burst_fps` to 10-15
- **Longer videos**: Use H264 codec for smaller file size
- **Reconnect**: Click "Reconnect" button after changing URL

## Features at a Glance

| Feature | How to Use |
|---------|-----------|
| Live Stream | Automatic on connection |
| FPS Display | Shows in top-left corner |
| Single Capture | Click 📷 button |
| Burst Mode | Hold ⏺ button |
| Video Record | Click 🎥 button |
| Settings | Click ⚙ button |
| Reconnect | Change URL and click "Reconnect" |

## File Naming

- Photos: `capture_20260828_143052_123.jpg`
- Burst: `burst_SESSION_0001.jpg`, `burst_SESSION_0002.jpg`, etc.
- Videos: User-specified name

## Ready to Go!

Your professional camera viewer is ready. Connect to your ESP32 camera and start capturing!

For detailed documentation, see `README.md`.
