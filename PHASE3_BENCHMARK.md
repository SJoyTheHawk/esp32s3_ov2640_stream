# Phase 3 Benchmark: Network Settings and Status

This benchmark verifies the authenticated settings API, NVS persistence, and
WiFi reconnection behavior on ESP32-S3 hardware. The complete benchmark was
verified on the target device, including changing the WiFi SSID and password,
reconnecting to the new network, and accessing the device at its new IP.

## Prerequisites

- Complete the Phase 2 login benchmark first.
- Flash the current firmware and open Serial Monitor at `115200`.
- Set `CAMERA_IP` to the address printed by the firmware.
- Use test network values that are known to be reachable. An invalid SSID or
  static IP can make the web UI unavailable until the device is reflashed with
  erased NVS or the configured network becomes available.

```bash
CAMERA_IP=192.168.1.224
curl -sS -c /tmp/esp32-camera.cookies -X POST "http://$CAMERA_IP/api/login" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'username=admin&password=admin'
```

Use the current NVS password if it is no longer `admin`.

## API Checks

### 1. Authentication is required

```bash
curl -i "http://$CAMERA_IP/api/settings"
curl -i "http://$CAMERA_IP/api/status"
```

Both requests must return HTTP `401` with a JSON error.

### 2. Read settings and status

```bash
curl -i -b /tmp/esp32-camera.cookies "http://$CAMERA_IP/api/settings"
curl -i -b /tmp/esp32-camera.cookies "http://$CAMERA_IP/api/status"
```

The settings response must include `wifi_ssid`, `wifi_password_set`,
`use_dhcp`, `static_ip`, `gateway`, and `subnet`. It must not disclose the WiFi
password. The status response must include uptime, connection state, current IP,
SSID, and RSSI.

### 3. Invalid values are rejected

```bash
curl -i -b /tmp/esp32-camera.cookies -X POST \
  "http://$CAMERA_IP/api/settings" \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data 'wifi_ssid=Test&use_dhcp=false&static_ip=bad&gateway=192.168.1.1&subnet=255.255.255.0'
```

The response must be HTTP `400`, and the active connection must remain intact.

### 4. Save and reconnect

The safest end-to-end check is the Network form in the web UI. Enter the known
test SSID, leave the password blank to retain the stored password, choose DHCP
or enter valid manual addresses, and select **Save network**.

Expected behavior:

- The page reports that settings were saved and warns that WiFi is reconnecting.
- Serial Monitor logs `[WEB] Network settings saved`, followed by
  `[WEB] Applying updated network settings` and the WiFi connection result.
- For a manual address, reconnect the browser at the address shown in the save
  response. For DHCP, use the new address printed in Serial Monitor or shown by
  the router.

### 5. Persistence after reboot

Power-cycle or reset the ESP32-S3. Confirm it reconnects without editing the
firmware, then log in at its current IP and repeat the authenticated settings
request. The saved SSID, network mode, and manual address values must match.

## Pass Criteria

Phase 3 passes when protected endpoints reject unauthenticated requests, valid
settings survive a reboot, invalid addresses do not alter the connection, and
both DHCP and manual-IP changes reconnect successfully on the target network.
