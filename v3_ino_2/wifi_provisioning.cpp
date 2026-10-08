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
      restartPending_(false), restartAtMs_(0), connectionDeadlineMs_(0),
      connectionTestInProgress_(false), connectionTestCompleted_(false),
      connectionTestSucceeded_(false) {}

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
        if (connectionTestInProgress_) {
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
    return F(R"HTML(<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Camera Setup</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:system-ui,sans-serif;background:#0a0e13;color:#e5e7eb;line-height:1.6;min-height:100vh;display:grid;place-items:center;padding:16px}main{width:100%;max-width:440px}.page-head{display:flex;justify-content:space-between;align-items:center;gap:12px}h1{font-size:1.25rem;font-weight:600;margin-bottom:8px;color:#fff}.lang-toggle{width:auto;background:#374151;border:0;border-radius:6px;color:#e5e7eb;cursor:pointer;font-size:.8rem;font-weight:600;padding:7px 10px}p{color:#6b7280;font-size:.875rem;margin-bottom:20px}form{background:#1a1f26;border:1px solid #2d3748;border-radius:8px;padding:20px;display:grid;gap:16px}label{display:grid;gap:6px;font-size:.875rem;font-weight:500;color:#cbd5e1}input,select{background:#0f1419;border:1px solid #2d3748;border-radius:6px;color:#e5e7eb;font-size:1rem;padding:10px;outline:none;transition:all 0.2s;font-family:inherit}input:focus,select:focus{border-color:#3b82f6}select{cursor:pointer}button[type=submit],#scan{background:#3b82f6;border:none;border-radius:6px;color:#fff;cursor:pointer;font-size:.875rem;font-weight:600;padding:10px 16px;transition:all 0.2s;font-family:inherit}button:hover:not(:disabled){background:#2563eb}button:disabled{opacity:0.5;cursor:not-allowed}#scan{background:#374151}#scan:hover:not(:disabled){background:#4b5563}.status{margin-top:16px;padding:10px;border-radius:4px;font-size:.875rem;display:none}.status.show{display:block}.status.info{background:rgba(59,130,246,0.15);color:#3b82f6}.status.error{background:#7f1d1d;color:#fca5a5}.status.success{background:#14532d;color:#86efac}.spinner{display:inline-block;width:14px;height:14px;border:2px solid currentColor;border-right-color:transparent;border-radius:50%;animation:spin 0.6s linear infinite;margin-right:8px;vertical-align:middle}@keyframes spin{to{transform:rotate(360deg)}}</style></head>
<body><main><div class="page-head"><h1 data-i18n="title">Camera Setup</h1><button type="button" id="language-toggle" class="lang-toggle">中文</button></div><p data-i18n="intro">Connect your camera to WiFi and configure access credentials.</p><form id="f"><label><span data-i18n="wifiNetwork">WiFi Network</span><input id="ssid" name="ssid" maxlength="31" data-i18n-placeholder="networkName" placeholder="Network name" required></label><button type="button" id="scan" data-i18n="scanNetworks">Scan Networks</button><select id="networks" hidden></select><label><span data-i18n="wifiPassword">WiFi Password</span><input name="password" type="password" maxlength="31" data-i18n-placeholder="openNetwork" placeholder="Leave blank if open network"></label><label><span data-i18n="adminUsername">Admin Username</span><input name="adminUsername" maxlength="31" value="admin" required></label><label><span data-i18n="adminPassword">Admin Password</span><input name="adminPassword" type="password" minlength="4" maxlength="31" data-i18n-placeholder="minimumPassword" placeholder="Minimum 4 characters" required></label><label><span data-i18n="userUsername">User Username</span><input name="userUsername" maxlength="31" value="user" required></label><label><span data-i18n="userPassword">User Password</span><input name="userPassword" type="password" minlength="4" maxlength="31" data-i18n-placeholder="minimumPassword" placeholder="Minimum 4 characters" required></label><button id="submit" type="submit" data-i18n="saveConnect">Save and Connect</button></form><div class="status" id="status"></div></main><script>
const i18n={en:{title:'Camera Setup',pageTitle:'Camera Setup',intro:'Connect your camera to WiFi and configure access credentials.',wifiNetwork:'WiFi Network',networkName:'Network name',scanNetworks:'Scan Networks',wifiPassword:'WiFi Password',openNetwork:'Leave blank if open network',adminUsername:'Admin Username',adminPassword:'Admin Password',userUsername:'User Username',userPassword:'User Password',minimumPassword:'Minimum 4 characters',saveConnect:'Save and Connect',switchToChinese:'中文',switchToEnglish:'English',scanning:'Scanning for networks...',networksFound:n=>n+' network(s) found',noNetworks:'No networks found. Try again.',scanFailed:'Scan failed. Check your connection.',connecting:'Connecting...',testing:'Testing WiFi credentials...',timeout:'Connection test timed out. Check credentials and retry.',connected:'Connected! Rebooting...',configurationFailed:'Configuration failed',couldNotGet:'Could not get test result. Check credentials and retry.'},zh:{title:'摄像头设置',pageTitle:'摄像头设置',intro:'连接摄像头到 WiFi 并配置访问凭据。',wifiNetwork:'WiFi 网络',networkName:'网络名称',scanNetworks:'扫描网络',wifiPassword:'WiFi 密码',openNetwork:'开放网络请留空',adminUsername:'管理员用户名',adminPassword:'管理员密码',userUsername:'用户用户名',userPassword:'用户密码',minimumPassword:'至少 4 个字符',saveConnect:'保存并连接',switchToChinese:'中文',switchToEnglish:'English',scanning:'正在扫描网络...',networksFound:n=>'找到 '+n+' 个网络',noNetworks:'未找到网络，请重试。',scanFailed:'扫描失败，请检查连接。',connecting:'连接中...',testing:'正在测试 WiFi 凭据...',timeout:'连接测试超时，请检查凭据后重试。',connected:'已连接！正在重启...',configurationFailed:'配置失败',couldNotGet:'无法获取测试结果，请检查凭据后重试。'}};
const languageKey='esp32CameraLanguage';const initialLanguage=()=>{try{const saved=localStorage.getItem(languageKey);if(saved==='en'||saved==='zh')return saved}catch(e){}return navigator.language&&navigator.language.toLowerCase().startsWith('zh')?'zh':'en'};let language=initialLanguage();const t=key=>typeof i18n[language][key]==='function'?i18n[language][key]:i18n[language][key]||i18n.en[key]||key;function applyLanguage(){document.documentElement.lang=language==='zh'?'zh-CN':'en';document.title=t('pageTitle');document.querySelectorAll('[data-i18n]').forEach(el=>el.textContent=t(el.dataset.i18n));document.querySelectorAll('[data-i18n-placeholder]').forEach(el=>el.placeholder=t(el.dataset.i18nPlaceholder));document.getElementById('language-toggle').textContent=language==='en'?t('switchToChinese'):t('switchToEnglish')}document.getElementById('language-toggle').onclick=()=>{language=language==='en'?'zh':'en';try{localStorage.setItem(languageKey,language)}catch(e){}applyLanguage()};applyLanguage();
const f=document.getElementById('f'),scan=document.getElementById('scan'),ssid=document.getElementById('ssid'),networks=document.getElementById('networks'),submit=document.getElementById('submit'),status=document.getElementById('status');let checkInterval,statusPollInterval;function showStatus(msg,type){status.textContent=msg;status.className='status show '+type}function startCheck(){const start=Date.now();checkInterval=setInterval(()=>{const elapsed=Math.floor((Date.now()-start)/1000);showStatus((language==='zh'?'正在检查连接... ':'Checking connection... ')+elapsed+'s','info')},1000)}function stopCheck(){if(checkInterval){clearInterval(checkInterval);checkInterval=null}if(statusPollInterval){clearInterval(statusPollInterval);statusPollInterval=null}}scan.onclick=async()=>{if(scan.disabled)return;scan.disabled=true;showStatus(t('scanning'),'info');try{const n=await(await fetch('/api/scan')).json();if(n.length){ssid.value=n[0].ssid;networks.innerHTML=n.map(x=>'<option>'+x.ssid+' ('+x.rssi+' dBm)</option>').join('');networks.hidden=false;networks.onchange=()=>ssid.value=networks.value.replace(/ \(.*$/,'');showStatus(t('networksFound')(n.length),'success')}else{showStatus(t('noNetworks'),'error')}}catch(e){showStatus(t('scanFailed'),'error')}finally{scan.disabled=false}};f.onsubmit=async e=>{e.preventDefault();if(submit.disabled)return;submit.disabled=true;submit.innerHTML='<span class="spinner"></span>'+t('connecting');showStatus(t('testing'),'info');startCheck();const formData=new URLSearchParams(new FormData(f));const startTime=Date.now();let pollAttempts=0;async function pollStatus(){if(Date.now()-startTime>15000){stopCheck();showStatus(t('timeout'),'error');submit.disabled=false;submit.textContent=t('saveConnect');return}pollAttempts++;try{const r=await fetch('/api/status',{method:'GET',signal:AbortSignal.timeout(2000)});const result=await r.json();if(result.testing){return}stopCheck();if(result.success){showStatus(result.message||t('connected'),'success')}else{showStatus(result.message||t('configurationFailed'),'error');submit.disabled=false;submit.textContent=t('saveConnect')}}catch(e){if(pollAttempts<30){return}stopCheck();showStatus(t('couldNotGet'),'error');submit.disabled=false;submit.textContent=t('saveConnect')}}try{await fetch('/api/configure',{method:'POST',body:formData});statusPollInterval=setInterval(pollStatus,500)}catch(error){statusPollInterval=setInterval(pollStatus,500)}};
</script></body></html>)HTML");
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
    if (connectionTestInProgress_) {
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
    connectionTestInProgress_ = true;
    connectionTestCompleted_ = false;
    connectionTestSucceeded_ = false;
    connectionTestMessage_ = "";
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
    request->send(202, "application/json", "{\"success\":true,\"message\":\"WiFi check started\"}");
}

void WiFiProvisioning::processConnection() {
    if (!connectionTestInProgress_) return;
    if (WiFi.status() != WL_CONNECTED && static_cast<long>(millis() - connectionDeadlineMs_) < 0) return;

    const bool connected = WiFi.status() == WL_CONNECTED;

    if (!connected) {
        Serial.printf("[PROVISION] WiFi connection failed (status: %d)\n", WiFi.status());
        connectionTestSucceeded_ = false;
        connectionTestMessage_ = "Could not connect; check WiFi credentials and retry";
        connectionTestCompleted_ = true;
        connectionTestInProgress_ = false;

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
        connectionTestSucceeded_ = false;
        connectionTestMessage_ = "Connected, but failed to save settings; retry";
        connectionTestCompleted_ = true;
        connectionTestInProgress_ = false;

        WiFi.disconnect(true);
        delay(200);
        WiFi.mode(WIFI_AP);
        delay(200);
        WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
        WiFi.softAP(AP_SSID, AP_PASSWORD);

        return;
    }

    Serial.println("[PROVISION] Settings saved. Rebooting...");
    connectionTestSucceeded_ = true;
    connectionTestMessage_ = "Connected. Rebooting...";
    connectionTestCompleted_ = true;
    connectionTestInProgress_ = false;
    restartPending_ = true;
    restartAtMs_ = millis() + 3000UL;
}
