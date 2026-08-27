# ESP32 Camera Python Clients

These optional clients consume the ESP32's standard authenticated MJPEG stream. They do not require firmware changes.

Install dependencies with `pip install -r requirements.txt`, then use:

```bash
python camera_viewer.py
python stream_viewer_simple.py --url http://CAMERA_IP/stream
python video_recorder.py --url http://CAMERA_IP/stream --output recordings/camera.avi
python photo_capture.py --url http://CAMERA_IP/stream --output captures
python motion_detector.py --url http://CAMERA_IP/stream
python cloud_uploader.py --directory captures --url https://example/upload
```

Replace `192.168.1.201` with the current camera IP. Set `ESP32_CAMERA_USERNAME` and `ESP32_CAMERA_PASSWORD` first, or add `--username admin --password YOUR_PASSWORD` to the viewer/recorder/capture/motion commands.

The ESP32 requires a login cookie. Set `ESP32_CAMERA_USERNAME` and `ESP32_CAMERA_PASSWORD`, or pass `--username` and `--password` to command-line clients. The GUI stores credentials and its URL, capture directory, auto-capture state, interval, and window size in `settings.json`. It captures one frame per second by default; use the toggle to pause or resume automatic capture.
