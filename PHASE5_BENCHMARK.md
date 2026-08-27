# Phase 5 Benchmark: Camera Settings UI

This benchmark verifies the authenticated camera configuration API, live
preview, real-time sensor controls, camera reinitialization, and NVS
persistence on ESP32-S3 hardware. The API, invalid-input, valid-configuration,
snapshot, and persistence checks were verified on the target device.

## Prerequisites

- Complete the Phase 2 login and Phase 3 network benchmarks.
- Flash the Phase 5 firmware with a working OV2640 camera and PSRAM enabled.
- Set `CAMERA_IP` to the current device address and log in:

```bash
CAMERA_IP=192.168.1.224
curl -sS -c /tmp/esp32-camera.cookies -X POST "http://$CAMERA_IP/api/login" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'username=admin&password=YOUR_PASSWORD'
```

## UI Checks

Open `http://$CAMERA_IP/` in a browser with the authenticated session.

Expected:

- The Camera section shows a live preview from `/stream`.
- Resolution offers QVGA, VGA, SVGA, XGA, and UXGA.
- JPEG quality ranges from 10 to 63.
- Frame rate offers 5, 10, 15, and 20 FPS.
- Brightness, contrast, and saturation each range from -2 to +2.
- Vertical flip and horizontal mirror are available as checkboxes.
- The current values are loaded from the device.

## API Checks

### 1. Authentication is required

```bash
curl -i -X POST "http://$CAMERA_IP/api/camera/config" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'resolution=9&quality=20&frame_rate=10&brightness=0&contrast=0&saturation=0&vertical_flip=false&horizontal_mirror=false'
```

The request must return HTTP `401`. A `GET` request is not a supported method
for this endpoint and correctly returns HTTP `404`.

### 2. Invalid values are rejected

```bash
curl -i -b /tmp/esp32-camera.cookies -X POST \
  "http://$CAMERA_IP/api/camera/config" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'resolution=99&quality=1&frame_rate=7&brightness=9&contrast=0&saturation=0&vertical_flip=false&horizontal_mirror=false'
```

The response must return HTTP `400` with `Invalid camera setting range`.

### 3. Apply a valid configuration

```bash
curl -i -b /tmp/esp32-camera.cookies -X POST \
  "http://$CAMERA_IP/api/camera/config" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'resolution=9&quality=20&frame_rate=10&brightness=1&contrast=-1&saturation=1&vertical_flip=true&horizontal_mirror=false'
```

Expected:

- HTTP `200` with `Camera settings applied`.
- The live preview changes to VGA dimensions and the image orientation/colour
  responds to the requested sensor settings.
- Serial output reports camera reinitialization when resolution or quality
  changed.

### 4. Confirm returned settings

```bash
curl -sS -b /tmp/esp32-camera.cookies \
  "http://$CAMERA_IP/api/settings"
```

The camera fields must report resolution `9`, quality `20`, frame rate `10`,
brightness `1`, contrast `-1`, saturation `1`, vertical flip `true`, and
horizontal mirror `false`.

## Snapshot and Frame Rate

Capture a frame after each resolution change:

```bash
curl -sS -b /tmp/esp32-camera.cookies \
  "http://$CAMERA_IP/capture" -o phase5-capture.jpg
file phase5-capture.jpg
```

The file must be a valid JPEG at the selected resolution. For FPS testing,
observe `/stream` for at least 30 seconds at 5, 10, 15, and 20 FPS. The stream
must remain connected and its cadence must change with the selected value.

## Persistence

Reset the ESP32-S3, log in again at its current IP, and repeat `GET /api/settings`.
All selected camera values must survive the reboot and the live preview must
start with the saved configuration.

## Pass Criteria

Phase 5 passes when the UI and API expose all camera controls, invalid values
are rejected, valid settings apply without reboot, snapshots match the selected
resolution, stream cadence follows the selected FPS, and all values persist
after reboot.
