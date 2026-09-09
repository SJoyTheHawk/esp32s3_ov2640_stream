# HX711 Implementation Code Guide

**Companion to:** `HX711_WEIGHT_METADATA_PLAN.md`
**Purpose:** Concrete implementation instructions, including a copy-ready web
panel change guide. Where this guide and the plan disagree, this guide wins.

## What this guide corrects in the plan

1. Tare must be a **runtime** operation persisted to NVS, not a compile-time
   constant. Without it the only way to zero the scale is to reflash.
2. `html_pages.h` must be edited. The plan omits it, so the MVP would ship with
   no way to zero the scale from the panel.
3. The frame-capture callback signature must change; see section 6.
4. `widgets/stream_widget.py`, `utils/file_manager.py`, and
   `test_stream_worker.py` must be edited. The plan omits them.
5. The HX711 read must run inside a critical section (section 5).

## 1. Permission model

| Endpoint | Method | Admin | User | Notes |
| --- | --- | --- | --- | --- |
| `/api/weight` | GET | yes | yes | read-only |
| `/api/weight/tare` | POST | yes | yes | operational zero |
| `/api/weight/calibrate` | POST | yes | no | 403 for user |

Tare is deliberately available to the `user` role. Operators zero an empty
platform as part of normal work; requiring admin credentials for that makes the
device unusable in the field. Calibration changes the scale factor permanently
and stays admin-only.

Use the existing helpers in `web_server.cpp`: `isAuthenticated()` for tare and
read, `isAdminAuthenticated()` for calibrate. Follow the established 403/401
pattern from `handlePostSettings` (`web_server.cpp:264-268`):

```cpp
if (!isAdminAuthenticated(request)) {
    if (isAuthenticated(request)) sendJson(request, 403, "Admin access required");
    else sendUnauthorized(request);
    return;
}
```

## 2. API contract

`GET /api/weight` — mirror `handleGetStatus` structure, `StaticJsonDocument<256>`:

```json
{
  "grams": 1275.4,
  "raw": 823441,
  "age_ms": 84,
  "valid": true,
  "stable": true,
  "offset": 812340,
  "scale": 42.7
}
```

`age_ms` is required. `millis()` timestamps are meaningless to a client with no
shared epoch, so the firmware computes the age itself. Omit `grams`/`raw` when
`valid` is false rather than sending a stale value.

`POST /api/weight/tare` — no body. Averages N samples (use 16), stores the mean
as the new offset, persists to NVS, responds:

```json
{"status":"success","message":"Tare complete","offset":812340}
```

Return 503 with `"Scale not responding"` if readings are invalid. Never tare
from an invalid reading — that bakes garbage into NVS permanently.

`POST /api/weight/calibrate` — form-encoded `known_grams`, matching the
`application/x-www-form-urlencoded` convention every other POST uses. Validate
`known_grams > 0` and reject if the raw delta from offset is near zero:

```json
{"status":"success","message":"Calibration saved","scale":42.7}
```

## 3. Settings / NVS additions

In `camera_settings.h`, add to `DefaultValues`:

```cpp
static constexpr int32_t WEIGHT_OFFSET = 0;
static constexpr float WEIGHT_SCALE = 1.0f;
```

Add public fields `int32_t weightOffset; float weightScale;` and methods
`bool writeWeightOffset(int32_t offset);` and
`bool writeWeightScale(float scale);`.

NVS keys must be 15 characters or fewer. Use `wtOffset` and `wtScale`. Follow
the exact existing pattern — `prefs.begin(NVS_NAMESPACE, false)`, write,
`prefs.end()`:

```cpp
bool CameraSettings::writeWeightOffset(int32_t offset) {
    if (!prefs.begin(NVS_NAMESPACE, false)) return false;
    const bool ok = prefs.putInt("wtOffset", offset) > 0;
    prefs.end();
    if (ok) weightOffset = offset;
    return ok;
}
```

`writeWeightScale` uses `putFloat` and must reject non-finite or zero values
before writing. Add both keys to `initializeNVS()`, add reads to
`readFromNVS()` with `getInt`/`getFloat`, add both to `setDefaults()`, and add a
line to `printSettings()`.

A scale factor of `1.0` means uncalibrated. The panel must surface that state
rather than displaying a meaningless gram value.

## 4. Files to change

Firmware: `weight_sensor.h`, `weight_sensor.cpp` (new), `v3_ino_2.ino`,
`web_server.h`, `web_server.cpp`, **`html_pages.h`**, `camera_settings.h`,
`camera_settings.cpp`.

Python: `mjpeg_stream.py`, `professional_viewer/mjpeg_stream.py`,
`workers/stream_worker.py`, `workers/save_worker.py`,
**`widgets/stream_widget.py`**, **`utils/file_manager.py`**,
`camera_viewer_pro.py`, **`test_stream_worker.py`**.

## 5. HX711 read timing (do not skip)

The HX711 powers down if SCK stays high longer than ~60 us. The sampling task
runs alongside WiFi (priority ~23), the camera task (priority 2, pinned to
core 0), and AsyncTCP. A preempted read yields silent garbage, not an error.

Wrap the 25-pulse shift-in in a critical section and pin the task to core 1:

```cpp
static portMUX_TYPE hx711Mux = portMUX_INITIALIZER_UNLOCKED;

portENTER_CRITICAL(&hx711Mux);
// 25 clock pulses, ~2 us each
portEXIT_CRITICAL(&hx711Mux);
```

That disables interrupts for roughly 50-100 us, which is safe for the camera's
DMA path. Check `digitalRead(DT) == LOW` for readiness *outside* the critical
section, and time out after 250 ms into `valid = false`.

Task: priority 1, core 1, 4096-byte stack.

## 6. Frame association

`web_server.h:18` and `:33` currently declare:

```cpp
std::function<size_t(uint8_t*, size_t)> frameCaptureCallback_;
```

The weight snapshot must be copied inside the same `frameMutex` critical
section as the JPEG (`v3_ino_2.ino:84-91`), so the callback needs an
out-parameter:

```cpp
std::function<size_t(uint8_t*, size_t, WeightReading&)>
```

Update `setFrameCaptureCallback`, `captureJpeg`, `handleStream`, and
`handleCapture`. Reading a shared global from the HTTP task instead is exactly
the race the plan forbids.

Mark a reading invalid when its age exceeds 1000 ms. A dead HX711 otherwise
leaves the last good value frozen and plausible.

In `handleStream`, add the headers to the existing `state->prefix` string built
at `web_server.cpp:498`, in the same block that captures the frame. Do not add
work to the chunked-response lambda beyond string building — it already blocks
the AsyncTCP task with a `delay()` at line 491.

## 7. Web panel change guide

All edits are in `html_pages.h`, inside the `MAIN[]` raw string literal. The
`LOGIN[]` literal is not touched.

### 7.0 Rules for editing this file

The existing style is deliberate. Match it or the page will look bolted-on.

- CSS lives in minified single-line blocks. Keep that. Do not reformat
  existing lines — it makes the diff unreviewable.
- The literal is `R"HTML(...)HTML"`. The sequence `)HTML"` must never appear in
  your markup.
- No external resources. No CDN, no fonts, no icon libraries. The device serves
  this page with no internet.
- Design tokens already in use — reuse exactly, do not invent new colors:
  - page background `#0a0e13`, card `#1a1f26`, input `#0f1419`
  - border `#2d3748`, primary `#3b82f6`, focus border `#3b82f6`
  - text `#e5e7eb`, label `#cbd5e1`, muted `#6b7280`
  - ok `#86efac` on `#14532d`, error `#fca5a5` on `#7f1d1d`,
    notice `#fcd34d` on `#78350f`
- Reuse existing classes: `section`, `metric`, `grid`, `full`, `check`,
  `actions`, `btn-primary`, `quiet`, `danger`, `message`, `ok`, `error`,
  `notice`, `muted`, `hidden`, `modal`, `modal-content`, `modal-header`,
  `close-btn`, `range`.
- `.admin-only` is toggled by JS at line 81. Anything admin-restricted gets
  that class and nothing else.
- Responsive collapse is handled by the existing
  `@media(max-width:768px)` rule. Grid children need no extra work.

### 7.1 New CSS

Append one line to the end of the `<style>` block (after the `.preview`/`.range`
line). This is the only CSS addition.

```css
.weight-hero{display:flex;align-items:baseline;gap:10px;flex-wrap:wrap;margin-bottom:14px}.weight-hero b{font-variant-numeric:tabular-nums;font-size:2.6rem;font-weight:700;color:#fff;line-height:1}.weight-hero i{font-style:normal;font-size:1.1rem;color:#6b7280}.pill{margin-left:auto;padding:4px 10px;border-radius:999px;font-size:.7rem;font-weight:700;text-transform:uppercase;letter-spacing:.5px;border:1px solid transparent}.pill-ok{color:#86efac;background:#14532d;border-color:#166534}.pill-wait{color:#fcd34d;background:#78350f;border-color:#92400e}.pill-bad{color:#fca5a5;background:#7f1d1d;border-color:#991b1b}.weight-raw{display:flex;gap:18px;flex-wrap:wrap;padding-top:12px;border-top:1px solid #2d3748;color:#6b7280;font-size:.78rem}.weight-raw span{font-variant-numeric:tabular-nums}.weight-raw span b{color:#cbd5e1;font-weight:600}
```

`font-variant-numeric:tabular-nums` matters. Without it the digits change width
as the value updates and the readout visibly jitters at 1 Hz.

The pill uses `margin-left:auto` so it right-aligns in the flex row without a
wrapper element.

### 7.2 Weight section markup

Insert this **between** the camera `</section>` (end of line 53) and the
`<section class="admin-only">` device-settings block (line 54). Weight sits
above device settings because it is operational, not configuration.

```html
<section><h2>Scale <span class="pill pill-wait" id="w-pill">Waiting</span></h2>
<div class="weight-hero"><b id="w-grams">--</b><i>g</i></div>
<div class="actions"><button type="button" class="btn-primary" id="w-tare">Tare (zero)</button><button type="button" class="quiet admin-only" id="w-cal-open">Calibrate</button><p id="w-message" class="message"></p></div>
<div class="weight-raw"><span>Raw <b id="w-raw">-</b></span><span>Age <b id="w-age">-</b></span><span>Offset <b id="w-offset">-</b></span><span>Scale <b id="w-scale">-</b></span></div>
</section>
```

Notes on this markup:

- The pill is inside `<h2>`, which is `display:block` with `margin:0 0 12px`.
  The `margin-left:auto` needs a flex parent, so the `<h2>` for this section
  must get inline `style="display:flex;align-items:center"`. Alternatively drop
  the pill next to the hero value. Pick one and be consistent.
- `Tare (zero)` is `btn-primary` and **not** `admin-only`. It is the primary
  action for both roles.
- `Calibrate` carries `admin-only` and uses `quiet`, matching the
  Configure/Change buttons in device settings.
- The raw strip is diagnostic. Keep it muted and small; it must not compete
  with the hero number.

Word the button "Tare (zero)" rather than "Tare". Operators who have not used a
load cell before do not know the term.

### 7.3 Calibration modal

Insert after the `password-modal` block (after line 66), before `<script>`.
Structure copied from the existing modals so keyboard/backdrop close works with
no extra JS.

```html
<div id="cal-modal" class="modal" aria-hidden="true"><div class="modal-content" role="dialog" aria-modal="true" aria-labelledby="cal-title"><div class="modal-header"><h2 id="cal-title">Calibrate scale</h2><button type="button" class="close-btn" data-close="cal-modal" aria-label="Close">&times;</button></div><form id="cal-form">
<p class="muted">Tare with an empty platform first, then place a known mass and enter its weight.</p>
<label for="known">Known mass (grams)</label><input id="known" name="known_grams" type="number" min="0.1" step="0.1" required>
<div class="actions"><button type="button" class="quiet" data-close="cal-modal">Cancel</button><button type="submit" class="btn-primary">Save calibration</button><p id="cal-message" class="message"></p></div></form></div></div>
```

The `data-close` attribute is all that is needed for the Cancel and X buttons —
the existing delegated handler at line 73 picks them up automatically. Backdrop
click (line 74) and Escape (line 75) also work with no changes.

### 7.4 JavaScript

Add before the final `load();` call at line 94.

**Critical constraint:** keep this entirely out of `load()`. `load()` is one
`try` block (lines 81-85) — if a weight request throws inside it, WiFi status,
camera settings, and the admin sections all silently stop populating. Poll
weight on its own timer with its own error handling.

```js
const wPill=document.getElementById('w-pill'),wMsg=document.getElementById('w-message');
const wSet=(cls,text)=>{wPill.className='pill '+cls;wPill.textContent=text};
let wBusy=false;
async function pollWeight(){if(wBusy||document.hidden)return;wBusy=true;
try{const d=await (await api('/api/weight')).json();
document.getElementById('w-grams').textContent=d.valid?Number(d.grams).toFixed(1):'--';
document.getElementById('w-raw').textContent=d.valid?d.raw:'-';
document.getElementById('w-age').textContent=d.valid?d.age_ms+' ms':'-';
document.getElementById('w-offset').textContent=d.offset;
document.getElementById('w-scale').textContent=Number(d.scale).toFixed(3);
if(!d.valid)wSet('pill-bad','No sensor');
else if(Number(d.scale)===1)wSet('pill-wait','Uncalibrated');
else if(!d.stable)wSet('pill-wait','Settling');
else wSet('pill-ok','Stable')}
catch(e){wSet('pill-bad','Offline');document.getElementById('w-grams').textContent='--'}
finally{wBusy=false}}
setInterval(pollWeight,1000);pollWeight();
document.getElementById('w-tare').onclick=async()=>{const b=document.getElementById('w-tare');b.disabled=true;wMsg.className='message';wMsg.textContent='Taring...';
try{const r=await api('/api/weight/tare',{method:'POST'});const d=await r.json();wMsg.className='message '+(r.ok?'ok':'error');wMsg.textContent=d.message||'Request failed'}
catch(e){wMsg.className='message error';wMsg.textContent='Request failed'}
finally{b.disabled=false;pollWeight();setTimeout(()=>{wMsg.textContent='';wMsg.className='message'},4000)}};
document.getElementById('w-cal-open').onclick=()=>openModal('cal-modal');
document.getElementById('cal-form').onsubmit=async(e)=>{e.preventDefault();const cm=document.getElementById('cal-message');const body=new URLSearchParams(new FormData(e.target));
if(!(Number(body.get('known_grams'))>0)){cm.className='message error';cm.textContent='Enter a mass greater than zero';return}
cm.className='message';cm.textContent='Calibrating...';
try{const r=await api('/api/weight/calibrate',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json();
cm.className='message '+(r.ok?'ok':'error');cm.textContent=d.message||'Request failed';
if(r.ok){pollWeight();setTimeout(()=>closeModal('cal-modal'),900)}}
catch(e){cm.className='message error';cm.textContent='Request failed'}};
```

Why it is written this way:

- **1000 ms, not 500 ms.** The panel already holds an MJPEG connection open via
  the `<img>` preview at line 69. AsyncWebServer on an ESP32 has a small socket
  pool; polling faster competes with the stream for it.
- **`wBusy` guard.** Prevents overlapping requests piling up if the device is
  slow to answer, which would exhaust the socket pool.
- **`document.hidden` check.** Stops polling in a background tab. Free win on a
  device this constrained.
- **Reuses `api()`** (line 77), so a 401 redirects to login exactly like every
  other call. Do not use bare `fetch`.
- **Button disabled during tare.** Double-tapping tare mid-operation writes NVS
  twice; NVS has finite erase cycles.
- **`toFixed(1)`.** The hero must render a fixed decimal count or it jitters.
- Message auto-clears after 4 s, matching the transient-status feel of the
  Python viewer's status bar.

### 7.5 Modal close handler

`closeModal` (line 71) has per-modal reset branches. Add one for the new modal:

```js
if(id==='cal-modal'){const cm=document.getElementById('cal-message');cm.textContent='';cm.className='message';document.getElementById('cal-form').reset()}
```

Without this, a stale error message is still on screen the next time the modal
opens.

### 7.6 What the user role sees

`load()` line 81 hides every `.admin-only` element for the user role. Result:

- **Admin:** Scale card with live value, pill, Tare, Calibrate, raw strip; plus
  Device Settings.
- **User:** Scale card with live value, pill, Tare, raw strip. No Calibrate
  button, no Device Settings.

Since `w-cal-open` is inside the card but the card itself is not `admin-only`,
the card renders correctly for both roles with no extra logic. Verify the
`actions` row still looks balanced with one button — it will, because `actions`
is `flex` with `gap:10px` and no `justify-content`.

Client-side hiding is presentation only. The server must still return 403 for a
user hitting `/api/weight/calibrate` directly.

### 7.7 Panel acceptance checks

- Admin sees Tare and Calibrate; user sees only Tare.
- User calling `/api/weight/calibrate` directly gets 403, not 200.
- Pill reads `Uncalibrated` on a factory-reset device (scale == 1.0).
- Pill reads `No sensor` with DT disconnected, and the hero shows `--`,
  never a stale number.
- Tare on a disconnected sensor fails with an error and does not write NVS.
- Camera preview keeps streaming while weight polls.
- WiFi/IP/uptime metrics still populate if `/api/weight` returns 500.
- Layout holds at 375 px width.
- Escape and backdrop click close the calibration modal.
- Session expiry during polling redirects to login once, not in a loop.

## 8. Python client changes

Beyond the plan's list:

- `widgets/stream_widget.py` — `update_frame` receives `StreamFrame`, stores the
  metadata, and `get_current_frame()` returns image plus metadata. This is the
  only source for `capture_single` (`camera_viewer_pro.py:196`) and
  `capture_burst_frame` (`:223`), so per-frame weight is impossible without it.
- `workers/stream_worker.py` — `frame_ready` becomes `pyqtSignal(object)`.
- `utils/file_manager.py` — add a sidecar path helper
  (`filepath.with_suffix('.json')`).
- `workers/save_worker.py` — queue `(frame, filepath, metadata)` and write the
  sidecar after `cv2.imwrite`. Write the JPEG first; a sidecar without an image
  is worse than an image without a sidecar.
- `test_stream_worker.py:25,63` asserts the current `frame_ready` payload and
  breaks the moment the signal type changes.

Video recording (`camera_viewer_pro.py:266`) loses per-frame weight. Either make
that an explicit non-goal or write a per-frame CSV alongside the `.avi`.

## 9. Suggested order

1. `camera_settings` offset/scale + NVS. Verify persistence across reboot.
2. `weight_sensor` with the critical section. Verify over Serial only.
3. `/api/weight`, then tare, then calibrate. Verify with curl.
4. Web panel (section 7). Zero the scale from a browser before touching
   the stream path.
5. Callback signature change + MJPEG headers. Verify browser stream unbroken.
6. Python parser, then widget, then save path.

Step 4 before step 5 is deliberate: it gets you a working, calibratable scale
before any change touches the streaming path that all existing clients depend
on.


