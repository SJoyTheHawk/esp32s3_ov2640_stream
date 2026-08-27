# ESP32-S3 Camera System Implementation Plan

**Project:** ESP32-S3 + OV2640 Camera with Web UI and MJPEG Streaming  
**Reference:** Gate_V1.0 architecture pattern  
**Date:** 2026-08-27  
**Storage:** ESP32-S3 NVS (Preferences library)

---

## 🎯 Project Goals

1. **Standalone ESP32-S3 camera system** with full web-based configuration
2. **MJPEG streaming** using standard protocol (not custom HTTP POST per frame)
3. **Authentication system** (login, logout, change password)
4. **WiFi configuration** via web UI (SSID, password, DHCP/manual IP)
5. **Camera settings** configurable in real-time (resolution, quality, flip/mirror, etc.)
6. **Optional Python server** for recording and cloud upload features
7. **Dual-core architecture** for responsive streaming and UI

---

## 📋 Implementation Phases

### **Phase 1: Storage Migration (EEPROM → NVS)**
**Goal:** Replace emulated EEPROM with ESP32-S3 native Preferences library

**Tasks:**
- [x] Create `CameraSettings` class based on Gate's `Setting` class
- [x] Define settings structure:
  ```cpp
  - username / password (login credentials)
  - wifiSSID / wifiPassword
  - useDHCP / staticIP / gateway / subnet
  - cameraResolution (QVGA/VGA/SVGA/XGA/UXGA)
  - cameraQuality (10-63)
  - frameRate (5/10/15/20 fps)
  - brightness / contrast / saturation
  - verticalFlip / horizontalMirror
  - pythonServerEnabled / pythonServerIP
  ```
- [x] Implement Preferences read/write methods
- [x] Add `initializeNVS()` to set defaults on first boot
- [x] Add `isNVSInitialized()` check
- [x] Test settings persistence across reboots (representative settings verified on ESP32-S3)

**Files to create:**
- `camera_settings.h`
- `camera_settings.cpp`

**Estimated time:** 4-6 hours

---

### **Phase 2: Web Server + Authentication**
**Goal:** Port Gate's login system and AsyncWebServer structure

**Tasks:**
- [x] Setup AsyncWebServer on port 80
- [x] Implement authentication endpoints:
  - `POST /api/login` - validate credentials, set cookie
  - `POST /api/logout` - clear session cookie
  - `POST /api/change-password` - update password in NVS
- [x] Implement cookie-based session management (30min timeout)
- [x] Create `isAuthenticated()` middleware function
- [x] Port Gate's login HTML page
- [x] Port Gate's main configuration HTML page (remove gate-specific parts)
- [ ] Test login/logout flow (follow `PHASE2_BENCHMARK.md` on ESP32-S3 hardware)

**Files to create:**
- `web_server.h`
- `web_server.cpp`
- `html_pages.h` (embedded HTML as raw strings)

**Estimated time:** 6-8 hours

---

### **Phase 3: Settings Management API**
**Goal:** Implement web UI for network and system settings

**Tasks:**
- [ ] Implement settings endpoints:
  - `GET /api/settings` - return current configuration as JSON
  - `POST /api/settings` - update and persist settings
  - `GET /api/status` - uptime, IP, connection status
- [ ] Add network configuration form to web UI:
  - WiFi SSID / password
  - DHCP vs Manual IP selection
  - Static IP / Gateway / Subnet inputs
- [ ] Add WiFi reconnection logic on settings change
- [ ] Add IP address change handling (warn user about connection loss)
- [ ] Test all settings changes and persistence

**Files to modify:**
- `web_server.cpp` (add endpoints)
- `html_pages.h` (add settings forms)

**Estimated time:** 6-8 hours

---

### **Phase 4: MJPEG Streaming Server**
**Goal:** Implement proper MJPEG stream endpoint on ESP32

**Tasks:**
- [ ] Remove current HTTP POST per frame logic
- [ ] Implement MJPEG streaming endpoint:
  - `GET /stream` - multipart/x-mixed-replace stream
- [ ] Implement frame boundary formatting:
  ```
  --frame\r\n
  Content-Type: image/jpeg\r\n\r\n
  [JPEG data]
  \r\n
  ```
- [ ] Add frame rate control (5/10/15/20 fps)
- [ ] Handle multiple simultaneous clients
- [ ] Add client connection/disconnection tracking
- [ ] Implement `GET /capture` for single snapshot
- [ ] Test stream with browser and VLC

**Files to modify:**
- `web_server.cpp`

**Estimated time:** 4-6 hours

---

### **Phase 5: Camera Settings UI**
**Goal:** Add camera configuration to web interface

**Tasks:**
- [ ] Add camera settings section to HTML:
  - Resolution dropdown (QVGA/VGA/SVGA/XGA/UXGA)
  - JPEG Quality slider (10-63)
  - Frame Rate selector (5/10/15/20 fps)
  - Brightness slider (-2 to +2)
  - Contrast slider (-2 to +2)
  - Saturation slider (-2 to +2)
  - Vertical Flip checkbox
  - Horizontal Mirror checkbox
- [ ] Implement camera configuration endpoint:
  - `POST /api/camera/config` - apply settings in real-time
- [ ] Add live preview to web UI:
  - `<img src="/stream">` element
  - FPS counter display
- [ ] Implement camera reinitialization on resolution change
- [ ] Test all camera settings changes

**Files to modify:**
- `html_pages.h` (add camera settings form)
- `web_server.cpp` (add camera config endpoint)
- `camera_settings.cpp` (apply camera settings)

**Estimated time:** 6-8 hours

---

### **Phase 6: Dual-Core Architecture**
**Goal:** Split workload across ESP32-S3 cores for better performance

**Tasks:**
- [ ] Create FreeRTOS tasks structure:
  - **Core 0:** Camera capture and streaming (high priority)
  - **Core 1:** Web server and settings management (lower priority)
- [ ] Implement thread-safe frame sharing:
  - Create mutex for shared frame buffer
  - Core 0 captures and updates latest frame
  - Core 1 reads frame for client streaming
- [ ] Implement task functions:
  ```cpp
  void cameraTask(void *parameter);  // Core 0
  void networkTask(void *parameter); // Core 1
  ```
- [ ] Move camera loop to `cameraTask` on Core 0
- [ ] Move web server loop to `networkTask` on Core 1
- [ ] Empty main `loop()` function (all work in tasks)
- [ ] Test stability under load (multiple clients, settings changes)
- [ ] Monitor task stack usage and adjust if needed

**Files to modify:**
- Main `.ino` file (restructure setup/loop)
- `web_server.cpp` (thread-safe frame access)

**Estimated time:** 4-6 hours

---

### **Phase 7: Python Server Integration (Optional)**
**Goal:** Add optional Python server connection for recording/cloud features

**Tasks:**
- [ ] Add Python server settings to web UI:
  - Enable/disable toggle
  - Server IP address input
  - Connection status indicator
- [ ] Implement polling mechanism:
  - `GET /api/poll` endpoint on ESP32
  - ESP32 polls Python server every 2 seconds
  - Python server returns commands (if any)
- [ ] Update Python server.py:
  - Change `/stream` from POST receiver to HTTP client pulling from ESP32
  - Add `/commands` endpoint for ESP32 to poll
  - Update recording to pull from ESP32 stream
- [ ] Implement command handling on ESP32:
  - LED flash (photo taken)
  - Resolution change
  - Status query
- [ ] Test Python server connection (connect/disconnect)
- [ ] Test recording functionality

**Files to create/modify:**
- ESP32: `python_client.cpp` (polling logic)
- Python: `server.py` (update architecture)

**Estimated time:** 6-8 hours

---

### **Phase 8: Additional Features**
**Goal:** Polish and usability improvements

**Tasks:**
- [ ] Add mDNS support:
  - Access via `http://camera.local`
  - Implement hostname configuration in settings
- [ ] Add OTA (Over-The-Air) updates:
  - ArduinoOTA library
  - Password-protected
  - Update via `camera.local` or IP
- [ ] Add status LED indicators:
  - WiFi connecting (slow blink)
  - WiFi connected (solid)
  - Streaming active (fast blink)
  - Error state (fast double-blink)
- [ ] Add system restart endpoint:
  - `POST /api/restart`
  - Requires authentication
- [ ] Add factory reset mechanism:
  - Long-press button (if available)
  - Or web UI button with confirmation
- [ ] Improve error handling:
  - WiFi connection failures
  - Camera initialization errors
  - Settings validation
- [ ] Add web UI enhancements:
  - Real-time FPS display
  - Connection quality indicator
  - Storage usage (if SD card added)

**Estimated time:** 8-10 hours

---

### **Phase 9: Testing & Documentation**
**Goal:** Comprehensive testing and user documentation

**Tasks:**
- [ ] **Functional Testing:**
  - Login/logout flow
  - All settings changes persist correctly
  - WiFi configuration and reconnection
  - Camera settings apply correctly
  - MJPEG streaming stability
  - Multiple client connections (3-5 browsers)
  - Python server optional connection
- [ ] **Performance Testing:**
  - Measure frame rates at different resolutions
  - Test under continuous 24-hour operation
  - Monitor memory usage and leaks
  - Test WiFi reconnection on router restart
- [ ] **Security Testing:**
  - Cookie expiration works correctly
  - Password change flow secure
  - No credential leakage in logs
- [ ] **Documentation:**
  - Create USER_GUIDE.md
  - Document all API endpoints
  - Create wiring diagram
  - Document default credentials
  - Create troubleshooting guide
- [ ] **Code cleanup:**
  - Remove debug Serial prints or add debug mode
  - Add code comments
  - Organize files in logical structure

**Estimated time:** 8-12 hours

---

## 📂 Project File Structure

```
esp32s3_ov2640_v3/
├── esp32_camera_v4/               # New Arduino project
│   ├── esp32_camera_v4.ino        # Main file (setup, dual-core tasks)
│   ├── camera_settings.h          # Settings class definition
│   ├── camera_settings.cpp        # NVS read/write implementation
│   ├── web_server.h               # AsyncWebServer class
│   ├── web_server.cpp             # All endpoints and handlers
│   ├── html_pages.h               # Embedded HTML pages
│   ├── camera_control.h           # Camera init and control functions
│   ├── camera_control.cpp
│   ├── python_client.h            # Optional Python server polling
│   ├── python_client.cpp
│   └── pin_definitions.h          # Hardware pin mappings
│
├── server.py                      # Updated Python server (optional)
├── IMPLEMENTATION_PLAN.md         # This file
├── USER_GUIDE.md                  # To be created
└── API_REFERENCE.md               # To be created
```

---

## 🔧 Technical Specifications

### **Hardware Requirements**
- ESP32-S3 development board
- OV2640 camera module
- USB power supply (5V 2A recommended)
- Optional: Status LED, Reset button, SD card module

### **Software Requirements**
- Arduino IDE 2.x or PlatformIO
- ESP32 board support (Arduino-ESP32 v2.0.11+)
- Libraries:
  - `esp_camera` (built-in)
  - `ESPAsyncWebServer`
  - `AsyncTCP`
  - `Preferences` (built-in)
  - `WiFi` (built-in)
  - `ESPmDNS` (built-in)
  - `ArduinoOTA` (optional)

### **Default Configuration**
```
Default Credentials:
- Username: admin
- Password: admin

Default WiFi:
- SSID: `XIMS2`
- Password: `Ns203Ns203.`
- AP Mode: ESP32-Camera-XXXXXX (if no WiFi configured)

Default Camera:
- Resolution: SVGA (800x600)
- Quality: 12
- Frame Rate: 20 fps

Default Network:
- DHCP: Enabled
- mDNS: camera.local
```

---

## 🎯 Success Criteria

### **Functional Requirements**
- ✅ User can access web UI via browser on local network
- ✅ Authentication system prevents unauthorized access
- ✅ All settings persist across power cycles
- ✅ MJPEG stream viewable in any browser
- ✅ Camera settings apply in real-time
- ✅ System runs stable for 24+ hours
- ✅ Multiple users can view stream simultaneously (3-5 clients)

### **Performance Requirements**
- ✅ 20 fps at SVGA resolution with 1 client
- ✅ 15+ fps at SVGA with 3 clients
- ✅ Web UI responds within 500ms
- ✅ Settings changes apply within 2 seconds
- ✅ WiFi reconnects automatically within 30 seconds

### **Usability Requirements**
- ✅ First-time setup clear and simple
- ✅ Accessible via mDNS hostname
- ✅ All settings explained with hints in UI
- ✅ Error messages are clear and actionable

---

## ⏱️ Time Estimates

| Phase | Description | Est. Hours |
|-------|-------------|------------|
| 1 | Storage Migration | 4-6 |
| 2 | Web Server + Auth | 6-8 |
| 3 | Settings API | 6-8 |
| 4 | MJPEG Streaming | 4-6 |
| 5 | Camera Settings UI | 6-8 |
| 6 | Dual-Core Architecture | 4-6 |
| 7 | Python Integration | 6-8 |
| 8 | Additional Features | 8-10 |
| 9 | Testing & Docs | 8-12 |
| **Total** | | **52-72 hours** |

---

## 🚀 Next Steps

1. **Review and approve this plan**
2. **Set up development environment**
3. **Start with Phase 1: Storage Migration**
4. **Commit to git after each phase completion**
5. **Test thoroughly before moving to next phase**

---

## 📝 Notes

- **Ethernet support commented out** - Can be added later if needed
- **Python server is optional** - ESP32 works standalone
- **Follow Gate_V1.0 patterns** - Proven architecture
- **Security:** Cookie-based auth is fine for local network use
- **Future:** Consider SD card for local photo/video storage

---

## 🔗 References

- Current implementation: `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/v3_ino_2/v3_ino_2.ino`
- Reference architecture: `/Users/szemy/Workspace/Gate/Gate_V1.0/Gate_V1.0.ino`
- Python server: `/Users/szemy/Workspace/Genican/esp32s3_ov2640_v3/server.py`

---

**Plan Status:** ✅ Ready for Implementation  
**Last Updated:** 2026-08-27
