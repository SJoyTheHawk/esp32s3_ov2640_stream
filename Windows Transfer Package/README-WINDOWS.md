# ESP32-S3 Camera Viewer for Windows

This folder is the Windows hand-off package for the professional MJPEG viewer.
Transfer the complete folder to a Windows PC; do not copy individual files.

## First run

1. Install Python 3.10+ and enable **Add Python to PATH**.
2. Run `Install Dependencies.bat`.
3. Run `Professional Viewer.bat`.
4. Configure the ESP32 URL in Settings, for example:
   `http://192.168.2.100/stream`.

The launcher starts from the package directory, so the bundled imports work
without installing the project as a Python package. The default capture folder
is `captures` beside this README.

## Captures and weight metadata

Each photo is saved as a JPEG. When the ESP32 provides HX711 metadata, the
viewer also writes a JSON sidecar with the same base name:

```text
captures/capture_YYYYMMDD_HHMMSS_fff.jpg
captures/capture_YYYYMMDD_HHMMSS_fff.json
```

If the scale is unavailable, the sidecar records `weight_valid: false`.

## Troubleshooting

- If dependencies fail, rerun `Install Dependencies.bat` from this folder.
- If the stream does not connect, verify the ESP32 IP, `/stream` path, Wi-Fi,
  and login credentials.
- Do not move `camera_viewer_pro.py` out of the `professional_viewer` folder.
