# Captive Portal Workflow Diagrams

## Scenario 1: Correct WiFi Credentials

### Client Side (Browser)
```
[User fills form with correct credentials]
    ↓
[User clicks "Save and Connect"]
    ↓
[JavaScript: Disable submit button]
    ↓
[JavaScript: Show spinner + "Connecting..."]
    ↓
[JavaScript: Display "Testing WiFi credentials..." (info)]
    ↓
[JavaScript: Start countdown timer (0s, 1s, 2s...)]
    ↓
[Send POST to /api/configure with form data]
    ↓
[Wait for response... timer keeps counting]
    ↓
[Receive HTTP 200 with success:true]
    ↓
[JavaScript: Stop timer]
    ↓
[JavaScript: Display "Connected. Rebooting..." (success)]
    ↓
[Connection to AP will be lost as ESP32 reboots]
    ↓
[User's device switches to the configured WiFi]
```

### ESP32 Side (Firmware)
```
[AP Mode active: "ESP32-CAM-Setup" at 192.168.4.1]
    ↓
[User browser connects to AP]
    ↓
[DNS Server redirects all requests to 192.168.4.1]
    ↓
[User loads http://192.168.4.1/ → setupPage()]
    ↓
[User submits form]
    ↓
[AsyncWebServer receives POST /api/configure]
    ↓
[handleConfigure() validates form fields]
    ├─ Check all required fields present
    ├─ Check SSID length (1-31 chars)
    ├─ Check password length (0-31 chars)
    ├─ Check admin/user credentials (4-31 chars)
    └─ Check usernames not duplicate
    ↓
[Store credentials in pending variables]
    ↓
[Set connectionDeadlineMs = now + 15000ms]
    ↓
[Serial: "Testing WiFi: <SSID>"]
    ↓
[WiFi.mode(WIFI_AP_STA) - Switch to dual mode]
    ↓
[delay(100ms) - Let mode switch stabilize]
    ↓
[WiFi.softAPConfig() - Reconfigure AP]
    ↓
[WiFi.softAP() - Restart AP with same settings]
    ↓
[delay(100ms) - Let AP stabilize]
    ↓
[WiFi.begin(SSID, password) - Start STA connection]
    ↓
[Return from handleConfigure() - request stays open]
    ↓
[Loop continues calling handleDNS()]
    ↓
[handleDNS() → processConnection() on each loop]
    ↓
[processConnection() checks WiFi.status()]
    ├─ If not WL_CONNECTED and time < deadline: return (keep waiting)
    └─ If WL_CONNECTED or timeout: proceed
    ↓
[WiFi.status() == WL_CONNECTED ✓]
    ↓
[Serial: "WiFi connected! IP: x.x.x.x"]
    ↓
[Save settings to NVS]
    ├─ writeWiFiSettings()
    ├─ writeAdminUsername()
    ├─ writeAdminPassword()
    ├─ writeUserUsername()
    ├─ writeUserPassword()
    └─ setWiFiConfigured(true)
    ↓
[Check if all writes succeeded]
    ↓
[All writes OK ✓]
    ↓
[Serial: "Settings saved. Rebooting..."]
    ↓
[Send HTTP 200 response]
    Body: {"success":true,"message":"Connected. Rebooting..."}
    ↓
[Set restartPending = true]
    ↓
[Set restartAtMs = now + 3000ms]
    ↓
[Continue loop for 3 seconds]
    ↓
[handleDNS() detects restartAtMs reached]
    ↓
[ESP.restart()]
    ↓
[ESP32 reboots]
    ↓
[setup() runs again]
    ↓
[settings.checkWiFiConfigured() returns true]
    ↓
[Skip AP provisioning mode]
    ↓
[connectWiFi() - Connect to saved SSID]
    ↓
[Normal operation begins]
```

---

## Scenario 2: Wrong WiFi Credentials

### Client Side (Browser)
```
[User fills form with WRONG password]
    ↓
[User clicks "Save and Connect"]
    ↓
[JavaScript: Disable submit button]
    ↓
[JavaScript: Show spinner + "Connecting..."]
    ↓
[JavaScript: Display "Testing WiFi credentials..." (info)]
    ↓
[JavaScript: Start countdown timer (0s, 1s, 2s...)]
    ↓
[Send POST to /api/configure with form data]
    ↓
[Wait for response... timer keeps counting]
    ↓
[Timer shows: 1s, 2s, 3s... up to ~15s]
    ↓
[Receive HTTP 200 with success:false]
    ↓
[JavaScript: Stop timer]
    ↓
[JavaScript: Display "Could not connect; check WiFi credentials and retry" (error)]
    ↓
[JavaScript: Re-enable submit button]
    ↓
[JavaScript: Change button text back to "Save and Connect"]
    ↓
[User can retry with corrected credentials]
```

### ESP32 Side (Firmware)
```
[AP Mode active: "ESP32-CAM-Setup" at 192.168.4.1]
    ↓
[User browser connects to AP]
    ↓
[DNS Server redirects all requests to 192.168.4.1]
    ↓
[User loads http://192.168.4.1/ → setupPage()]
    ↓
[User submits form with WRONG password]
    ↓
[AsyncWebServer receives POST /api/configure]
    ↓
[handleConfigure() validates form fields]
    ├─ All fields present ✓
    ├─ All lengths valid ✓
    └─ Usernames not duplicate ✓
    ↓
[Store credentials in pending variables]
    ↓
[Set connectionDeadlineMs = now + 15000ms]
    ↓
[Serial: "Testing WiFi: <SSID>"]
    ↓
[WiFi.mode(WIFI_AP_STA) - Switch to dual mode]
    ↓
[delay(100ms) - Let mode switch stabilize]
    ↓
[WiFi.softAPConfig() - Reconfigure AP]
    ↓
[WiFi.softAP() - Restart AP with same settings]
    ↓
[delay(100ms) - Let AP stabilize]
    ↓
[WiFi.begin(SSID, WRONG_password) - Start STA connection]
    ↓
[Return from handleConfigure() - request stays open]
    ↓
[Loop continues calling handleDNS()]
    ↓
[handleDNS() → processConnection() on each loop]
    ↓
[processConnection() checks WiFi.status()]
    ├─ Loop 1: status = WL_DISCONNECTED, time < deadline → return
    ├─ Loop 2: status = WL_DISCONNECTED, time < deadline → return
    ├─ Loop 3: status = WL_DISCONNECTED, time < deadline → return
    ├─ ... (continues for ~15 seconds)
    ├─ Loop N: status = WL_CONNECT_FAILED or WL_NO_SSID_AVAIL
    └─ Eventually: timeout reached (millis() >= connectionDeadlineMs)
    ↓
[WiFi.status() != WL_CONNECTED ✗]
    ↓
[Serial: "WiFi connection failed (status: <code>)"]
    ↓
[WiFi.disconnect(true) - Clean up failed connection]
    ↓
[delay(100ms)]
    ↓
[WiFi.mode(WIFI_AP) - Switch back to AP-only mode]
    ↓
[delay(100ms)]
    ↓
[WiFi.softAPConfig() - Reconfigure AP]
    ↓
[WiFi.softAP() - Restart AP]
    ↓
[AP is now stable again at 192.168.4.1]
    ↓
[Send HTTP 200 response]
    Body: {"success":false,"message":"Could not connect; check WiFi credentials and retry"}
    ↓
[Browser receives response and displays error]
    ↓
[configureRequest_ = nullptr - Reset state]
    ↓
[AP continues running - user can retry]
```

---

## Key Differences Between Scenarios

### Correct Credentials
- **WiFi.status()**: Becomes `WL_CONNECTED` within 15 seconds
- **Response**: `{"success":true,"message":"Connected. Rebooting..."}`
- **ESP32 Action**: Saves settings to NVS → Reboots → Connects to configured WiFi
- **User Experience**: Green success message → Device reboots → Normal operation
- **AP State**: Shuts down on reboot (no longer needed)

### Wrong Credentials
- **WiFi.status()**: Remains `WL_DISCONNECTED`, `WL_CONNECT_FAILED`, or `WL_NO_SSID_AVAIL`
- **Response**: `{"success":false,"message":"Could not connect; check WiFi credentials and retry"}`
- **ESP32 Action**: Restores AP mode → Keeps portal running → Waits for retry
- **User Experience**: Red error message → Button re-enabled → Can retry immediately
- **AP State**: Restored to stable operation (stays at 192.168.4.1)

---

## Critical Improvements in Fixed Version

### Problem in Old Version
```
[WiFi.mode(WIFI_AP_STA)]
    ↓
[WiFi.begin()] - AP becomes unstable here!
    ↓
[Browser loses connection to AP]
    ↓
[JavaScript catch(error)]
    ↓
[Shows wrong message: "The setup connection was lost..."]
```

### Solution in New Version
```
[WiFi.mode(WIFI_AP_STA)]
    ↓
[delay(100ms) - Stabilization time]
    ↓
[WiFi.softAPConfig() - Explicit reconfiguration]
    ↓
[WiFi.softAP() - Restart AP with same settings]
    ↓
[delay(100ms) - More stabilization]
    ↓
[WiFi.begin()] - AP stays stable now!
    ↓
[Browser maintains connection to AP]
    ↓
[Proper JSON response received]
    ↓
[Correct error message displayed]
```

### Why It Works
1. **Mode switch stabilization**: 100ms delay allows WiFi hardware to transition
2. **Explicit AP reconfiguration**: Forces AP back to correct IP/settings after mode change
3. **AP restart**: Ensures AP is broadcasting correctly in dual mode
4. **Longer timeout**: 15s instead of 10s gives more time for networks with slow DHCP
5. **Proper cleanup**: `WiFi.disconnect(true)` before mode changes prevents stale state
6. **Additional delays**: Between each transition prevents race conditions

---

## WiFi Status Codes Reference

When connection fails, `WiFi.status()` can return:
- `WL_IDLE_STATUS` (0) - WiFi is in idle mode
- `WL_NO_SSID_AVAIL` (1) - SSID cannot be reached
- `WL_CONNECT_FAILED` (4) - Password incorrect
- `WL_CONNECTION_LOST` (5) - Connection lost
- `WL_DISCONNECTED` (6) - Disconnected from AP

When connection succeeds:
- `WL_CONNECTED` (3) - Successfully connected

The code checks for `WL_CONNECTED` specifically. Any other status after timeout is treated as failure.
