# HX711 Weight Metadata for Camera Captures

**Status:** Proposal for review  
**Scope:** ESP32-S3 firmware, native MJPEG stream, and Python capture clients  
**Implementation state:** Design only; no feature code is included by this document

## Objective

Read a load cell through an HX711 connected to the ESP32-S3 and associate the
weight reading with the camera frame being displayed or saved.

The first implementation should preserve all existing MJPEG clients, avoid
blocking the camera or HTTP tasks, and make the captured weight available to
the Python applications without requiring JPEG parsing changes in ordinary
viewers.

## Proposed MVP

1. Add an HX711 reader and calibration configuration to the firmware.
2. Run HX711 sampling in its own low-priority FreeRTOS task.
3. Store the latest filtered weight reading in a small synchronized snapshot.
4. Copy that snapshot at the same time the camera task publishes `latestFrame`.
5. Add weight fields to each MJPEG multipart part as custom HTTP headers.
6. Add a JSON `GET /api/weight` endpoint for diagnostics and clients that do
   not consume MJPEG multipart headers.
7. Update Python parsing and capture code to retain the metadata.
8. Write a JSON sidecar next to each saved photo. Defer EXIF until the complete
   capture path has been validated.

Existing clients that only look for JPEG start/end markers will continue to
work because the image bytes and standard MJPEG content type remain unchanged.

## Hardware

The exact GPIOs must be confirmed against the specific ESP32-S3 board before
flashing. The current camera mapping occupies GPIOs 1-16, while GPIO 19 is used
for factory reset and GPIO 48 for the status LED. Candidate HX711 pins are
GPIO 17 and GPIO 18, subject to board schematic and boot-strapping checks.

```text
HX711 VCC -> ESP32-S3 3.3 V
HX711 GND -> ESP32-S3 GND
HX711 DT  -> selected GPIO input
HX711 SCK -> selected GPIO output
```

Power the HX711 at 3.3 V unless the selected board has a level-shifting design.
Do not expose a 5 V HX711 data output directly to an ESP32-S3 input.

## Weight data model

The firmware should use a fixed-size value type rather than passing the HX711
object through the web-server layer:

```cpp
struct WeightReading {
    float grams;
    int32_t raw;
    uint32_t sampledAtMs;
    bool valid;
    bool stable;
};
```

The reader task should:

- wait for HX711 data readiness without blocking the camera task;
- apply the configured tare and scale factor;
- use a short moving average or equivalent filter;
- publish the newest reading even when the value is not stable;
- mark readings invalid when the HX711 is disconnected or times out.

Calibration values should initially be compile-time constants or a small
firmware configuration block. NVS persistence can be added later after the
electrical and measurement behavior is confirmed.

## Frame association and synchronization

The current camera task copies a JPEG into `latestFrame`, protected by
`frameMutex`. Extend that same critical section to copy the current
`WeightReading` into a `latestFrameWeight` value.

The association rule is:

> A frame's weight is the most recent valid reading available when the camera
> task publishes that frame.

This avoids taking a new HX711 reading from an HTTP callback or from the Python
Save button, either of which could associate a weight from the wrong time.

The HTTP stream callback should capture one immutable frame metadata snapshot
when it starts sending a frame. It must not read fields that can change while
the JPEG bytes are being sent.

## MJPEG protocol extension

Each multipart frame should retain the current standard headers and add:

```http
--frame
Content-Type: image/jpeg
Content-Length: 48231
X-Weight-Grams: 1275.4
X-Weight-Raw: 823441
X-Weight-Sample-Ms: 483920
X-Weight-Valid: 1
X-Weight-Stable: 1

<jpeg bytes>
```

If the reading is invalid, send `X-Weight-Valid: 0` and either omit the other
weight fields or send their last value only when the client also sees
`X-Weight-Valid: 0`. The recommended behavior is to omit stale values.

The `/capture` response should expose the same headers. Add:

```http
X-Weight-Grams: 1275.4
X-Weight-Raw: 823441
X-Weight-Sample-Ms: 483920
X-Weight-Valid: 1
X-Weight-Stable: 1
```

Custom headers are ignored by ordinary MJPEG viewers, so this remains
backward-compatible with browser `<img>` clients.

## Weight API

Add an authenticated endpoint:

```http
GET /api/weight
```

Example response:

```json
{
  "grams": 1275.4,
  "raw": 823441,
  "sample_ms": 483920,
  "valid": true,
  "stable": true
}
```

The endpoint is for diagnostics, calibration tools, and clients that need a
weight display independent of the stream. It is not the authoritative source
for a saved frame; the per-frame MJPEG metadata is authoritative for capture.

## Python client changes

Keep the existing `frames()` API unchanged for compatibility. Add a metadata-
aware iterator, for example:

```python
@dataclass(frozen=True)
class StreamFrame:
    image: np.ndarray
    jpeg: bytes
    weight_grams: float | None
    weight_raw: int | None
    weight_sample_ms: int | None
    weight_valid: bool
    weight_stable: bool
```

The parser must read multipart headers before locating the JPEG payload. The
existing JPEG-marker fallback may remain for compatibility, but it cannot
provide weight metadata and should produce `weight_valid=False`.

Pass `StreamFrame` through the professional viewer's stream widget and capture
paths. Single captures and burst captures must use the metadata belonging to
the frame being saved.

## File output

The MVP should write a JSON sidecar beside each photo:

```text
capture_20260908_142315_123.jpg
capture_20260908_142315_123.json
```

Example:

```json
{
  "weight_g": 1275.4,
  "weight_raw": 823441,
  "weight_sample_ms": 483920,
  "weight_valid": true,
  "weight_stable": true
}
```

Sidecars are preferable for the first release because they preserve exact
metadata, require no JPEG rewrite, and work with the current OpenCV save path.

## EXIF decision

EXIF is optional follow-up work, not part of the MVP. There is no standard EXIF
field for scale weight. If required later, encode a compact value in EXIF
`UserComment`, such as:

```text
WeightGrams=1275.4;WeightValid=1;WeightSampleMs=483920
```

That change should use Pillow or `piexif` and should preferably save the
original JPEG bytes rather than decode and re-encode through OpenCV.

## Planned file changes

Firmware:

- `v3_ino_2/weight_sensor.h` and `v3_ino_2/weight_sensor.cpp`
- `v3_ino_2/v3_ino_2.ino`
- `v3_ino_2/web_server.h`
- `v3_ino_2/web_server.cpp`

Python:

- `python_clients/mjpeg_stream.py`
- `python_clients/professional_viewer/mjpeg_stream.py`
- `python_clients/professional_viewer/workers/stream_worker.py`
- `python_clients/professional_viewer/camera_viewer_pro.py`
- `python_clients/professional_viewer/workers/save_worker.py`
- optionally `python_clients/photo_capture.py`

No changes are planned for authentication, camera settings, or the existing
JPEG frame format.

## Validation gates before release

### Electrical and sensor validation

- Confirm HX711 GPIOs against the board schematic.
- Confirm 3.3 V operation and common ground.
- Confirm the firmware reports invalid readings when DT is disconnected.
- Tare the empty platform and verify repeatable readings with a known mass.
- Record calibration factor, noise, update rate, and settling behavior.

### Firmware validation

- `/stream` remains viewable in a normal browser.
- Existing authenticated Python clients still decode frames.
- Each frame's `X-Weight-Sample-Ms` is no newer than the frame publication time.
- Camera streaming remains responsive while the HX711 is disconnected.
- `/api/weight` returns valid JSON and correct authentication responses.
- `/capture` returns both JPEG bytes and matching weight headers.

### Capture validation

- Single capture writes one JPEG and one matching JSON sidecar.
- Burst capture writes one sidecar per frame.
- Invalid readings are represented as invalid, never as a plausible stale weight.
- Restarting the Python client does not break the firmware stream.
- Existing photos without sidecars remain readable.

## Explicit non-goals for the first implementation

- No EXIF mutation in the ESP32 firmware.
- No weight encoded into JPEG pixels or filenames.
- No synchronous HX711 read from an HTTP callback.
- No NVS calibration UI until the sensor and wiring are validated.
- No changes to the optional Flask server unless a later workflow requires it.

## Review decisions needed

Before implementation, confirm:

1. The HX711 GPIO pair after checking the board schematic.
2. Desired unit (`grams` is the proposed wire unit).
3. Whether the stable flag is needed in the first release.
4. Whether JSON sidecars are acceptable as the MVP output.
5. The load-cell range and calibration procedure.

