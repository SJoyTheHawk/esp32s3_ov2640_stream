# ESP32-S3 OV2640 Camera

Firmware and an optional Python companion server for an ESP32-S3 camera using
an OV2640 sensor.

## Current Features

- OV2640 JPEG capture at QVGA, VGA, SVGA, XGA, or UXGA
- Configurable JPEG quality and frame rate
- WiFi connection with DHCP or static network configuration
- Optional HTTP POST frame streaming to the Python server
- Preferences (NVS) backed camera, network, authentication, and server settings
- Embedded web login with cookie sessions
- Password change persisted to NVS
- Temporary NVS persistence test mode

## Project Layout

```text
v3_ino_2/
  v3_ino_2.ino          ESP32-S3 sketch and camera loop
  camera_settings.*     Preferences/NVS settings layer
  web_server.*          Async web server and authentication
  html_pages.h          Embedded login and main pages
server.py               Optional Flask receiver, recorder, and control UI
IMPLEMENTATION_PLAN.md  Phased implementation plan
PHASE2_BENCHMARK.md     Web authentication test procedure
```

Captured photos, recordings, backup copies, Python caches, and local build
artifacts are intentionally excluded from Git by `.gitignore`.

## Hardware

The sketch targets an ESP32-S3 with an OV2640 camera. The current pin mapping
is defined near the top of `v3_ino_2/v3_ino_2.ino` and should be checked against
the specific camera board before flashing.

Enable PSRAM in the Arduino board configuration when the module provides it.

## Arduino Dependencies

Install these libraries through the Arduino IDE Library Manager:

- ESPAsyncWebServer by ESP32Async
- AsyncTCP by ESP32Async
- ArduinoJson by Benoit Blanchon

The ESP32 board package supplies `WiFi`, `Preferences`, and `esp_camera`.

## Firmware Setup

1. Open `v3_ino_2/v3_ino_2.ino` in Arduino IDE.
2. Select the ESP32-S3 board and the correct USB port.
3. Configure local WiFi and Python server values in `camera_settings.h` before
   flashing, or use the settings API when Phase 3 is implemented.
4. Flash the sketch and open Serial Monitor at `115200`.
5. Confirm the log reports WiFi connection and `[WEB] Server started on port 80`.

The default web login is `admin` / `admin` unless the NVS password was changed.

Do not commit real WiFi passwords, credentials, or public upload endpoints.
The current development sketch contains local values and must be redacted or
moved to an ignored local configuration header before publishing this project.

## Python Server

Install dependencies:

```bash
python3 -m pip install -r requirements.txt
```

Start the receiver on the local network:

```bash
python3 server.py --host 0.0.0.0 --port 8000
```

The ESP32 must point to the computer's LAN IP, not `localhost`. The web UI is
available at `http://<server-ip>:8000/`.

## Validation

- Phase 1 NVS persistence: use the guarded `NVS_TEST_WRITE` block in the sketch.
- Phase 2 authentication: follow [PHASE2_BENCHMARK.md](PHASE2_BENCHMARK.md).
- Later phases cover the settings API, native MJPEG streaming, camera UI, and
  dual-core architecture.

## License

No license has been selected yet.
