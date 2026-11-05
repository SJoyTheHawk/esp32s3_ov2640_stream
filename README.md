# The Pipe Camera

ESP32-S3 camera firmware with OV2640 sensor and optional Python companion server.

## Current release

`v2.0-youxin` preserves the current Youxin ESP32-S3 / OV2640 board baseline
before migration to another manufacturer. See the [release notes](docs/releases/v2.0-youxin.md).

## Features

- JPEG capture at QVGA through UXGA with configurable quality and frame rate
- WiFi with DHCP or static IP configuration
- HTTP POST streaming to Python server
- Embedded web UI with authentication and cookie sessions
- WS2812B LED control (configurable strip length and evenly spaced active LEDs + onboard status pixel)
- HX711 weight sensor integration with scale calibration
- Persistent settings storage (NVS)
- Web-based network and camera configuration
- Internationalization support (i18n)

## Quick Setup

### Hardware Requirements

- ESP32-S3 with OV2640 camera module
- PSRAM enabled in board configuration
- Optional: 102 WS2812B LEDs (DIN on GPIO 21) + onboard status LED (GPIO 48)
- Optional: HX711 weight sensor module

### Firmware Setup

1. **Install Arduino libraries** (via Library Manager):
   - ESPAsyncWebServer by ESP32Async
   - AsyncTCP by ESP32Async
   - ArduinoJson by Benoit Blanchon
   - Adafruit NeoPixel by Adafruit Industries

2. **Flash firmware**:
   - Open `v3_ino_2/v3_ino_2.ino` in Arduino IDE
   - Select `ESP32-S3 Dev Module` board and USB port
   - Enable PSRAM in Tools menu, use `OPI PSRAM`
   - Upload sketch
   - Monitor serial output at 115200 baud

3. **First boot - WiFi configuration**:
   - Device will create a WiFi hotspot (check serial output for SSID)
   - Connect to the hotspot from your phone or computer
   - A captive portal page will appear automatically
   - Enter your WiFi credentials and network settings
   - Device will restart and connect to your network

4. **Access web UI**:
   - Find device IP in serial output after connection
   - Open `http://<device-ip>` in browser
   - Default login: `admin` / `admin`
   - Change password and configure settings via web UI

### Python Server (Optional)

For frame recording and remote viewing:

```bash
pip install -r requirements.txt
python3 server.py --host 0.0.0.0 --port 8000
```

Point ESP32 to `http://<server-lan-ip>:8000` (not localhost). Access web UI at same address.

## Project Structure

```text
v3_ino_2/
  v3_ino_2.ino          Main ESP32-S3 sketch
  camera_settings.*     NVS settings management
  web_server.*          Web server and authentication
  html_pages.h          Embedded web UI
server.py               Flask receiver and control server
python_clients/         MJPEG viewer and utilities
Windows Transfer Package/  Standalone Windows viewer
docs/                   Documentation and guides
```

## Hardware Details

### Camera Pin Mapping

Verify pin configuration in `v3_ino_2/v3_ino_2.ino` matches your camera board before flashing.

### WS2812B LED Strip

- Configurable maximum LED count (physical strip length), default 100, with DIN on GPIO 21
- Configurable active LED count, default 100; valid range is 1 through the configured maximum
- Configure both counts under **Configure LED → Save hardware**; settings survive reboot
- Active LED positions are computed as integer i * maximum / active, starting at LED 0
  - Maximum 10, active 5 → LEDs 0, 2, 4, 6, 8
  - Maximum 10, active 3 → LEDs 0, 3, 6
  - Equal counts → every LED turns on; inactive positions are cleared
- Maximum count supports 1–1024 LEDs (a buffer resource guard); set it to the physical strip length
- Existing saved active counts are retained on upgrade. The new maximum defaults to 100,
  or the existing active count if it is greater than 100 (e.g. 102)
- Current limiting uses the active count plus an idle reserve for the entire configured strip
- API: GET /api/led returns led_count (active), led_max_count, and led_count_limit;
  POST /api/led/hardware requires led_count, led_max_count, and data_gpio
- Add 330–470Ω resistor in series with DIN
- Use external 5V power supply for full strip
- Brightness limited when powered from board
- Consider 3.3V→5V level shifter for signal reliability

Status LED (GPIO 48):
- Green flash: startup, successful photo/config
- Red flash (5×): factory reset triggered
- Magenta flash: unrecognized command

## License

No license selected.
