# The Pipe Camera

ESP32-S3 camera firmware with OV2640 sensor and optional Python companion server.

## Features

- JPEG capture at QVGA through UXGA with configurable quality and frame rate
- WiFi with DHCP or static IP configuration
- HTTP POST streaming to Python server
- Embedded web UI with authentication and cookie sessions
- WS2812B LED control (45-LED strip + onboard status pixel)
- HX711 weight sensor integration with scale calibration
- Persistent settings storage (NVS)
- Web-based network and camera configuration
- Internationalization support (i18n)

## Quick Setup

### Hardware Requirements

- ESP32-S3 with OV2640 camera module
- PSRAM enabled in board configuration
- Optional: 45 WS2812B LEDs (DIN on GPIO 21) + onboard status LED (GPIO 48)
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

- 45 LEDs with DIN on GPIO 21
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
