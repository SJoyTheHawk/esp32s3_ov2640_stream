# ESP32 UXGA Resolution Issue - Review Summary

## Problem

The ESP32 camera is configured through its web interface as UXGA
(`1600x1200`), but photos captured by the Python professional viewer are
`1024x768`.

## Device Information

```text
ESP32 IP: 192.168.2.44
MJPEG stream: http://192.168.2.44/stream
```

## Verified Behavior

Read-only checks against the ESP32 endpoints returned:

```text
/api/status:
camera_resolution = 12
camera_quality    = 12
frame_rate        = 5

/api/settings:
camera_resolution = 12
```

Actual image inspection returned:

```text
/capture: 1024x768
/stream:  12 decoded MJPEG frames, all 1024x768
```

The Python viewer does not resize the captured image. It retains the original
decoded stream frame and passes it directly to `cv2.imwrite`. Therefore, the
ESP32 is genuinely transmitting `1024x768` frames.

## Root Cause

The project assumes the following resolution values:

```text
8  = QVGA
9  = VGA
10 = SVGA
11 = XGA
12 = UXGA
```

The installed ESP32 Arduino 3.3.11 camera library actually defines these enum
values:

```text
6  = FRAMESIZE_QVGA
10 = FRAMESIZE_VGA
11 = FRAMESIZE_SVGA
12 = FRAMESIZE_XGA
15 = FRAMESIZE_UXGA
```

The firmware casts the submitted logical value directly to `framesize_t`.
Consequently, submitting `12`, which the application labels as UXGA, selects
`FRAMESIZE_XGA` and produces a `1024x768` image.

## Relevant Source Files

- `v3_ino_2/camera_settings.h`
- `v3_ino_2/html_pages.h`
- `v3_ino_2/web_server.cpp`
- `v3_ino_2/v3_ino_2.ino`
- `python_clients/professional_viewer/widgets/stream_widget.py`
- `python_clients/professional_viewer/workers/save_worker.py`

The camera enum used by the installed board package is declared in:

```text
~/Library/Arduino15/packages/esp32/tools/esp32s3-libs/3.3.11/
include/espressif__esp32-camera/driver/include/sensor.h
```

## Recommended Fix

Preserve the existing API and NVS logical values `8` through `12`, but stop
casting them directly to `framesize_t`.

Add an explicit conversion from the project's logical values to the camera
library constants:

```text
8  -> FRAMESIZE_QVGA
9  -> FRAMESIZE_VGA
10 -> FRAMESIZE_SVGA
11 -> FRAMESIZE_XGA
12 -> FRAMESIZE_UXGA
```

Use this conversion whenever the firmware initializes or reconfigures the
camera. This approach:

- Preserves the current web UI and API contract.
- Preserves settings already stored in NVS.
- Avoids depending on camera-library enum numbering.
- Corrects every currently mislabeled resolution, not only UXGA.

## Suggested Verification After the Fix

For each UI resolution, apply the setting and verify both `/capture` and the
first decoded `/stream` frame:

```text
QVGA -> 320x240
VGA  -> 640x480
SVGA -> 800x600
XGA  -> 1024x768
UXGA -> 1600x1200
```

Also reboot the ESP32 and confirm the selected resolution persists and still
produces the correct dimensions.

## Investigation Status

No firmware changes were made as part of this investigation. The endpoint
checks were read-only.
