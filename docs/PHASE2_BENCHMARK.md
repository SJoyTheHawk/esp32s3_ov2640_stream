# Phase 2 Benchmark: Web Server and Authentication

This benchmark verifies the ESP32 web server and authentication layer. It does
not benchmark MJPEG throughput or the settings API; those belong to later
phases.

## Prerequisites

- ESP32-S3 firmware compiled with `ESPAsyncWebServer`, `AsyncTCP`, and
  `ArduinoJson` installed.
- ESP32 connected to WiFi.
- Serial Monitor open at `115200`.
- The ESP32 IP address, shown by the startup log.

Use this shell variable on the computer running the tests:

```bash
CAMERA_IP=192.168.1.224
```

The IP must be replaced if DHCP assigns a different address.

## Startup Check

After flashing, Serial Monitor must contain both lines:

```text
[WIFI] Connected! IP: <camera-ip>
[WEB] Server started on port 80
```

If the second line is missing, stop the benchmark and resolve WiFi or library
installation problems first.

## HTTP Acceptance Tests

### 1. Login page is public

```bash
curl -i "http://$CAMERA_IP/"
```

Expected:

- HTTP `200`
- `Content-Type` contains `text/html`
- Response contains `ESP32-S3 Camera` and a login form

### 2. Invalid credentials are rejected

```bash
curl -i -X POST "http://$CAMERA_IP/api/login" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'username=admin&password=wrong'
```

Expected:

- HTTP `401`
- JSON message contains `Invalid username or password`
- No usable `auth_token` cookie is issued

### 3. Valid credentials create a session

```bash
curl -i -c /tmp/esp32-camera.cookies -X POST "http://$CAMERA_IP/api/login" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'username=admin&password=admin'
```

Expected:

- HTTP `200`
- JSON status is `success`
- Response contains `Set-Cookie: auth_token=...`
- Serial Monitor logs `[WEB] User 'admin' logged in`

### 4. Authenticated root returns the main page

```bash
curl -i -b /tmp/esp32-camera.cookies "http://$CAMERA_IP/"
```

Expected:

- HTTP `200`
- Response contains `Authentication active` and `Change password`

### 5. Password change requires authentication and the current password

Unauthenticated request:

```bash
curl -i -X POST "http://$CAMERA_IP/api/change-password" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'current_password=admin&new_password=test1234'
```

Expected: HTTP `401`.

Authenticated request with the wrong current password:

```bash
curl -i -b /tmp/esp32-camera.cookies -X POST "http://$CAMERA_IP/api/change-password" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'current_password=wrong&new_password=test1234'
```

Expected: HTTP `401`.

### 6. Password change persists to NVS

```bash
curl -i -b /tmp/esp32-camera.cookies -X POST "http://$CAMERA_IP/api/change-password" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'current_password=admin&new_password=test1234'
```

Expected:

- HTTP `200`
- JSON message is `Password changed successfully`
- Serial Monitor logs `[WEB] Password changed`

Reboot the ESP32, then verify:

```bash
curl -i -c /tmp/esp32-camera-new.cookies -X POST "http://$CAMERA_IP/api/login" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'username=admin&password=test1234'
```

The login must return HTTP `200`. Restore the default password after testing if
this is not a production device.

### 7. Logout invalidates the session

```bash
curl -i -b /tmp/esp32-camera-new.cookies -c /tmp/esp32-camera-new.cookies \
  -X POST "http://$CAMERA_IP/api/logout"
curl -i -b /tmp/esp32-camera-new.cookies "http://$CAMERA_IP/"
```

Expected:

- Logout returns HTTP `200` and clears the cookie.
- The following root request returns the login page, not the main page.

## Session Timeout

The configured timeout is 30 minutes. For a fast validation, temporarily set
`COOKIE_TIMEOUT_MS` to `5000`, flash the firmware, log in, wait at least five
seconds, and request `/` with the saved cookie. The response must be the login
page and Serial Monitor must log `[WEB] Session expired`. Restore the 30-minute
value afterward.

## Pass Criteria

Phase 2 passes when all seven HTTP tests pass, password persistence works after
reboot, and the timeout test succeeds. A normal LAN response should be below
500 ms, but response time is network-dependent and is not the primary phase
acceptance criterion.
