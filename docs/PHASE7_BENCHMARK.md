# Phase 7 Benchmark: Python MJPEG Clients

Install dependencies:

```bash
cd python_clients
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

export ESP32_CAMERA_USERNAME=admin
export ESP32_CAMERA_PASSWORD='YOUR_CAMERA_PASSWORD'
```

With the ESP32 online, run each applicable client:

```bash
python camera_viewer.py
python stream_viewer_simple.py --url http://192.168.1.201/stream
python video_recorder.py --url http://192.168.1.201/stream --output recordings/test.avi --seconds 10
python photo_capture.py --url http://192.168.1.201/stream --output captures
python motion_detector.py --url http://192.168.1.201/stream
python cloud_uploader.py --directory captures --url https://YOUR_UPLOAD_ENDPOINT
```

In the GUI connection dialog, enter the camera URL, username, and password. Clicking or typing cancels the ten-second automatic countdown; the dialog then waits for the `Connect` button.

Expected results: the viewers show live video and FPS/resolution; the GUI captures one JPEG per second while enabled and persists its JSON settings; the recorder produces a playable AVI; photo capture saves on `s`; motion detection saves detected events; and the uploader reports each HTTP upload. Press `q` to stop interactive clients.

Static validation:

```bash
python3 -m py_compile *.py
```
