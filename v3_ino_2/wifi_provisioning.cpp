#include "wifi_provisioning.h"

#include <ArduinoJson.h>

namespace {
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
constexpr char AP_SSID[] = "ESP32-CAM-Setup";
constexpr char AP_PASSWORD[] = "12345678";
}

WiFiProvisioning::WiFiProvisioning(CameraSettings* settings)
    : settings_(settings), server_(80), startedAtMs_(0), active_(false),
      restartPending_(false), restartAtMs_(0), configureRequest_(nullptr),
      connectionDeadlineMs_(0), connectionTestCompleted_(false), connectionTestSucceeded_(false) {}

WiFiProvisioning::~WiFiProvisioning() { stopAPMode(); }

bool WiFiProvisioning::startAPMode() {
    if (!settings_) return false;
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET) ||
        !WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("[PROVISION] Failed to start AP");
        return false;
    }
    dnsServer_.start(DNS_PORT, "*", AP_IP);
    if (!MDNS.begin("camera")) {
        Serial.println("[PROVISION] mDNS startup failed");
    }
    setupWebHandlers();
    server_.begin();
    startedAtMs_ = millis();
    active_ = true;
    Serial.printf("[PROVISION] AP '%s' started at %s\n", AP_SSID,
                  WiFi.softAPIP().toString().c_str());
    return true;
}

void WiFiProvisioning::stopAPMode() {
    if (!active_) return;
    dnsServer_.stop();
    server_.end();
    WiFi.softAPdisconnect(true);
    MDNS.end();
    active_ = false;
}

void WiFiProvisioning::handleDNS() {
    if (active_) dnsServer_.processNextRequest();
    processConnection();
    if (restartPending_ && static_cast<long>(millis() - restartAtMs_) >= 0) {
        ESP.restart();
    }
}

bool WiFiProvisioning::isActive() const { return active_; }

bool WiFiProvisioning::timedOut() const {
    return active_ && static_cast<unsigned long>(millis() - startedAtMs_) >= AP_TIMEOUT_MS;
}

void WiFiProvisioning::setupWebHandlers() {
    server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html", setupPage());
    });
    server_.on("/api/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", scanNetworks());
    });
    server_.on("/api/configure", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleConfigure(request);
    });
    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // Return the current status of WiFi testing
        if (configureRequest_) {
            // Test is still in progress
            request->send(200, "application/json", "{\"testing\":true}");
        } else if (connectionTestCompleted_) {
            // Test completed, return the result
            String response = "{\"testing\":false,\"success\":";
            response += connectionTestSucceeded_ ? "true" : "false";
            response += ",\"message\":\"";
            response += connectionTestMessage_;
            response += "\"}";
            request->send(200, "application/json", response);
            // Reset test state after client receives result
            connectionTestCompleted_ = false;
            connectionTestSucceeded_ = false;
            connectionTestMessage_ = "";
        } else {
            // No test in progress and no completed test
            request->send(200, "application/json", "{\"testing\":false,\"success\":false,\"message\":\"Could not connect; check WiFi credentials and retry\"}");
        }
    });
    server_.onNotFound([this](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
}

String WiFiProvisioning::setupPage() const {
    return F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Camera Setup</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:system-ui,sans-serif;background:#0a0e13;color:#e5e7eb;line-height:1.6;min-height:100vh;display:grid;place-items:center;padding:16px}main{width:100%;max-width:440px}h1{font-size:1.25rem;font-weight:600;margin-bottom:8px;color:#fff}p{color:#6b7280;font-size:.875rem;margin-bottom:20px}form{background:#1a1f26;border:1px solid #2d3748;border-radius:8px;padding:20px;display:grid;gap:16px}label{display:grid;gap:6px;font-size:.875rem;font-weight:500;color:#cbd5e1}input,select{background:#0f1419;border:1px solid #2d3748;border-radius:6px;color:#e5e7eb;font-size:1rem;padding:10px;outline:none;transition:all 0.2s;font-family:inherit}input:focus,select:focus{border-color:#3b82f6}select{cursor:pointer}button{background:#3b82f6;border:none;border-radius:6px;color:#fff;cursor:pointer;font-size:.875rem;font-weight:600;padding:10px 16px;transition:all 0.2s;font-family:inherit}button:hover:not(:disabled){background:#2563eb}button:disabled{opacity:0.5;cursor:not-allowed}button[type=button]{background:#374151;color:#e5e7eb}button[type=button]:hover:not(:disabled){background:#4b5563}.status{margin-top:16px;padding:10px;border-radius:4px;font-size:.875rem;display:none}.status.show{display:block}.status.info{background:rgba(59,130,246,0.15);color:#3b82f6}.status.error{background:#7f1d1d;color:#fca5a5}.status.success{background:#14532d;color:#86efac}.spinner{display:inline-block;width:14px;height:14px;border:2px solid currentColor;border-right-color:transparent;border-radius:50%;animation:spin 0.6s linear infinite;margin-right:8px;vertical-align:middle}@keyframes spin{to{transform:rotate(360deg)}}</style></head><body><main><h1>Camera Setup</h1><p>Connect your camera to WiFi and configure access credentials.</p><form id='f'><label>WiFi Network <input id='ssid' name='ssid' maxlength='31' placeholder='Network name' required></label><button type='button' id='scan'>Scan Networks</button><select id='networks' hidden></select><label>WiFi Password <input name='password' type='password' maxlength='31' placeholder='Leave blank if open network'></label><label>Admin Username <input name='adminUsername' maxlength='31' value='admin' required></label><label>Admin Password <input name='adminPassword' type='password' minlength='4' maxlength='31' placeholder='Minimum 4 characters' required></label><label>User Username <input name='userUsername' maxlength='31' value='user' required></label><label>User Password <input name='userPassword' type='password' minlength='4' maxlength='31' placeholder='Minimum 4 characters' required></label><button id='submit'>Save and Connect</button></form><div class='status' id='status'></div></main><script>const f=document.getElementById('f'),scan=document.getElementById('scan'),ssid=document.getElementById('ssid'),networks=document.getElementById('networks'),submit=document.getElementById('submit'),status=document.getElementById('status');let checkInterval,statusPollInterval;function showStatus(msg,type){status.textContent=msg;status.className='status show '+type}function startCheck(){const start=Date.now();checkInterval=setInterval(()=>{const elapsed=Math.floor((Date.now()-start)/1000);showStatus('Checking connection... '+elapsed+'s','info')},1000)}function stopCheck(){if(checkInterval){clearInterval(checkInterval);checkInterval=null}if(statusPollInterval){clearInterval(statusPollInterval);statusPollInterval=null}}scan.onclick=async()=>{if(scan.disabled)return;scan.disabled=true;showStatus('Scanning for networks...','info');try{const n=await(await fetch('/api/scan')).json();if(n.length){networks.innerHTML=n.map(x=>`<option>${x.ssid} (${x.rssi} dBm)</option>`).join('');networks.hidden=false;networks.onchange=()=>ssid.value=networks.value.replace(/ \\(.*$/,'');showStatus(n.length+' network(s) found','success')}else{showStatus('No networks found. Try again.','error')}}catch(e){showStatus('Scan failed. Check your connection.','error')}finally{scan.disabled=false}};f.onsubmit=async e=>{e.preventDefault();if(submit.disabled)return;submit.disabled=true;submit.innerHTML='<span class=\"spinner\"></span>Connecting...';showStatus('Testing WiFi credentials...','info');startCheck();const formData=new URLSearchParams(new FormData(f));const startTime=Date.now();let pollAttempts=0;async function pollStatus(){if(Date.now()-startTime>15000){stopCheck();showStatus('Connection test timed out. Check credentials and retry.','error');submit.disabled=false;submit.textContent='Save and Connect';return}pollAttempts++;try{const r=await fetch('/api/status',{method:'GET',signal:AbortSignal.timeout(2000)});const result=await r.json();if(result.testing){return}stopCheck();if(result.success){showStatus(result.message||'Connected! Rebooting...','success')}else{showStatus(result.message||'Configuration failed','error');submit.disabled=false;submit.textContent='Save and Connect'}}catch(e){if(pollAttempts<30){return}stopCheck();showStatus('Could not get test result. Check credentials and retry.','error');submit.disabled=false;submit.textContent='Save and Connect'}}try{await fetch('/api/configure',{method:'POST',body:formData});statusPollInterval=setInterval(pollStatus,500)}catch(error){statusPollInterval=setInterval(pollStatus,500)}};</script></body></html>");
}

String WiFiProvisioning::scanNetworks() const {
    WiFi.mode(WIFI_AP_STA);
    const int count = WiFi.scanNetworks();
    DynamicJsonDocument document(2048);
    JsonArray networks = document.to<JsonArray>();
    for (int i = 0; i < count; ++i) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    String body;
    serializeJson(document, body);
    WiFi.scanDelete();
    WiFi.mode(WIFI_AP);
    return body;
}

void WiFiProvisioning::handleConfigure(AsyncWebServerRequest* request) {
    if (configureRequest_) {
        request->send(409, "application/json", "{\"success\":false,\"message\":\"A WiFi check is already in progress\"}");
        return;
    }
    const char* required[] = {"ssid", "password", "adminUsername", "adminPassword", "userUsername", "userPassword"};
    for (const char* name : required) {
        if (!request->hasParam(name, true)) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing setup field\"}");
            return;
        }
    }
    const String ssid = request->getParam("ssid", true)->value();
    const String wifiPassword = request->getParam("password", true)->value();
    const String adminUsername = request->getParam("adminUsername", true)->value();
    const String adminPassword = request->getParam("adminPassword", true)->value();
    const String userUsername = request->getParam("userUsername", true)->value();
    const String userPassword = request->getParam("userPassword", true)->value();
    if (ssid.length() == 0 || ssid.length() > 31 || wifiPassword.length() > 31 ||
        adminUsername.length() == 0 || adminUsername.length() > 31 || adminPassword.length() < 4 || adminPassword.length() > 31 ||
        userUsername.length() == 0 || userUsername.length() > 31 || userPassword.length() < 4 || userPassword.length() > 31 ||
        adminUsername == userUsername) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid or duplicate credentials\"}");
        return;
    }
    pendingSSID_ = ssid;
    pendingWiFiPassword_ = wifiPassword;
    pendingAdminUsername_ = adminUsername;
    pendingAdminPassword_ = adminPassword;
    pendingUserUsername_ = userUsername;
    pendingUserPassword_ = userPassword;
    configureRequest_ = request;
    connectionDeadlineMs_ = millis() + 12000UL;

    Serial.printf("[PROVISION] Testing WiFi: %s\n", pendingSSID_.c_str());

    // Clean up any previous station connection attempts
    WiFi.disconnect(true);
    delay(200);

    // Switch to AP+STA mode
    WiFi.mode(WIFI_AP_STA);
    delay(200);

    // Reconfigure the AP after mode change to ensure it stays stable
    if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
        Serial.println("[PROVISION] Failed to reconfigure AP");
    }
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("[PROVISION] Failed to restart AP");
    }
    delay(200);

    // Now begin the station connection test
    WiFi.begin(pendingSSID_.c_str(), pendingWiFiPassword_.c_str());
    Serial.println("[PROVISION] WiFi.begin() called, waiting for connection...");
}

void WiFiProvisioning::processConnection() {
    if (!configureRequest_) return;
    if (WiFi.status() != WL_CONNECTED && static_cast<long>(millis() - connectionDeadlineMs_) < 0) return;

    AsyncWebServerRequest* request = configureRequest_;
    configureRequest_ = nullptr;

    const bool connected = WiFi.status() == WL_CONNECTED;

    if (!connected) {
        Serial.printf("[PROVISION] WiFi connection failed (status: %d)\n", WiFi.status());

        // Send response BEFORE changing network modes to avoid request invalidation
        if (request) {
            request->send(200, "application/json", "{\"success\":false,\"message\":\"Could not connect; check WiFi credentials and retry\"}");
        }

        // Give time for response to be sent before network changes
        delay(100);

        // Clean disconnect
        WiFi.disconnect(true);
        delay(200);

        // Restore AP-only mode
        WiFi.mode(WIFI_AP);
        delay(200);

        // Reconfigure AP
        if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
            Serial.println("[PROVISION] Failed to reconfigure AP after failure");
        }
        if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
            Serial.println("[PROVISION] Failed to restart AP after failure");
        }

        Serial.printf("[PROVISION] AP restored at %s\n", WiFi.softAPIP().toString().c_str());
        return;
    }

    Serial.printf("[PROVISION] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

    if (!settings_->writeWiFiSettings(pendingSSID_.c_str(), pendingSSID_.length(), pendingWiFiPassword_.c_str(), pendingWiFiPassword_.length()) ||
        !settings_->writeAdminUsername(pendingAdminUsername_.c_str(), pendingAdminUsername_.length()) ||
        !settings_->writeAdminPassword(pendingAdminPassword_.c_str(), pendingAdminPassword_.length()) ||
        !settings_->writeUserUsername(pendingUserUsername_.c_str(), pendingUserUsername_.length()) ||
        !settings_->writeUserPassword(pendingUserPassword_.c_str(), pendingUserPassword_.length()) ||
        !settings_->setWiFiConfigured(true)) {
        Serial.println("[PROVISION] Failed to save settings to NVS");

        // Send response BEFORE changing network modes
        if (request) {
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Connected, but failed to save settings; retry\"}");
        }

        delay(100);

        WiFi.disconnect(true);
        delay(200);
        WiFi.mode(WIFI_AP);
        delay(200);
        WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
        WiFi.softAP(AP_SSID, AP_PASSWORD);

        return;
    }

    Serial.println("[PROVISION] Settings saved. Rebooting...");
    if (request) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Connected. Rebooting...\"}");
    }
    restartPending_ = true;
    restartAtMs_ = millis() + 3000UL;
}
