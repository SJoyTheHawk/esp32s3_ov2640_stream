#ifndef HTML_PAGES_H
#define HTML_PAGES_H

namespace CameraHtml {

static const char LOGIN[] = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Camera Login</title>
<style>
body{font-family:system-ui,sans-serif;background:#0b1220;color:#e5e7eb;display:grid;place-items:center;min-height:100vh;margin:0}
main{width:min(360px,calc(100% - 32px));padding:28px;background:#172033;border:1px solid #334155;border-radius:8px}
h1{font-size:1.35rem;margin:0 0 24px}label{display:block;margin:14px 0 6px;color:#94a3b8;font-size:.9rem}
input,button{width:100%;box-sizing:border-box;padding:11px;border-radius:5px;font:inherit}input{background:#0f172a;color:#e5e7eb;border:1px solid #475569}
button{margin-top:20px;border:0;background:#38bdf8;color:#082f49;font-weight:700;cursor:pointer}.error{color:#fca5a5;min-height:1.2em;margin-top:14px}
</style>
</head>
<body><main><h1>ESP32-S3 Camera</h1>
<form id="login"><label for="username">Username</label><input id="username" name="username" autocomplete="username" required autofocus>
<label for="password">Password</label><input id="password" name="password" type="password" autocomplete="current-password" required>
<button type="submit">Log in</button><div id="error" class="error"></div></form>
<script>
document.getElementById('login').addEventListener('submit',async(e)=>{e.preventDefault();
const form=new URLSearchParams(new FormData(e.target));const r=await fetch('/api/login',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:form});
const d=await r.json();if(r.ok){location.href='/'}else{document.getElementById('error').textContent=d.message||'Login failed'} });
</script></main></body></html>)HTML";

static const char MAIN[] = R"HTML(<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-S3 Camera</title>
<style>
*{box-sizing:border-box}body{font-family:system-ui,sans-serif;background:#0a0e13;color:#e5e7eb;margin:0;padding:16px}main{max-width:900px;margin:0 auto}
header{display:flex;justify-content:space-between;align-items:center;gap:16px;background:#1a1f26;border:1px solid #2d3748;border-radius:8px;padding:16px;margin-bottom:16px}h1{font-size:1.25rem;margin:0;color:#fff}h2{font-size:1rem;margin:0 0 12px;font-weight:600}
section{padding:16px;background:#1a1f26;border:1px solid #2d3748;border-radius:8px;margin-bottom:16px}.status{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin-bottom:16px}.metric{background:#1a1f26;border:1px solid #2d3748;border-radius:6px;padding:14px}.metric span{display:block;color:#6b7280;font-size:.7rem;text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px}.metric strong{display:block;font-size:1.1rem;color:#fff;overflow-wrap:anywhere}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.full{grid-column:1/-1}label{display:block;margin:12px 0 6px;color:#cbd5e1;font-size:.875rem}input,select{width:100%;padding:10px;background:#0f1419;color:#e5e7eb;border:1px solid #2d3748;border-radius:6px;font:inherit}input:focus,select:focus{outline:none;border-color:#3b82f6}
.check{display:flex;align-items:center;gap:9px;color:#e5e7eb;padding:8px 0}.check input{width:18px;height:18px;margin:0;cursor:pointer;accent-color:#3b82f6}.actions{display:flex;align-items:center;gap:10px;margin-top:20px;flex-wrap:wrap}button{padding:10px 16px;border:0;border-radius:6px;font-weight:600;font-size:.875rem;cursor:pointer}.btn-primary{background:#3b82f6;color:#fff;width:100%}.btn-primary:hover{background:#2563eb}.quiet{background:#374151;color:#e5e7eb}.quiet:hover{background:#4b5563}.danger{background:#dc2626;color:#fff}.danger:hover{background:#b91c1c}
.message{min-height:1.3em;margin:10px 0 0;padding:10px;border-radius:4px;font-size:.875rem}.error{color:#fca5a5;background:#7f1d1d}.ok{color:#86efac;background:#14532d}.notice{color:#fcd34d;background:#78350f}.muted{color:#6b7280;font-size:.8rem;margin:4px 0 0}.hidden{display:none}.modal{display:none;position:fixed;inset:0;z-index:100;background:rgba(0,0,0,.8);padding:16px;align-items:center;justify-content:center}.modal.show{display:flex}.modal-content{width:min(500px,100%);max-height:90vh;overflow:auto;background:#1a1f26;border:1px solid #2d3748;border-radius:8px;padding:20px}.modal-header{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:16px;padding-bottom:12px;border-bottom:1px solid #2d3748}.modal-header h2{margin:0}.close-btn{margin:0;padding:0;width:28px;height:28px;background:transparent;border:0;color:#9ca3af;font-size:1.5rem;line-height:1;cursor:pointer}.close-btn:hover{color:#fff}.setting-links{display:grid;grid-template-columns:1fr 1fr;gap:14px}.setting-link{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:12px 0;border-bottom:1px solid #2d3748}.setting-link:last-child{border-bottom:0}.setting-link strong{color:#f3f4f6}.setting-link p{margin:4px 0 0}@media(max-width:768px){body{padding:8px}.grid,.status,.setting-links{grid-template-columns:1fr}.full{grid-column:auto}}
.preview{width:100%;background:#000;border:1px solid #2d3748;border-radius:6px;display:block;margin-bottom:16px}.range{display:flex;align-items:center;gap:10px;margin-top:8px}.range input{flex:1}input[type="range"]{height:4px;background:#2d3748;border-radius:2px;outline:none;-webkit-appearance:none}input[type="range"]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;background:#3b82f6;border-radius:50%;cursor:pointer}input[type="range"]::-moz-range-thumb{width:16px;height:16px;background:#3b82f6;border:0;border-radius:50%;cursor:pointer}.range output{min-width:35px;padding:4px 8px;background:#2d3748;color:#3b82f6;border-radius:4px;font-size:.875rem;text-align:center}
</style></head>
<body><main><header><div><h1>ESP32-S3 Camera</h1><p class="muted">Logged in as <strong id="role">Loading</strong></p></div><button id="logout" class="danger">Log out</button></header>
<section class="status"><div class="metric"><span>WiFi</span><strong id="wifi-state">Loading</strong></div><div class="metric"><span>IP address</span><strong id="ip-address">-</strong></div><div class="metric"><span>Uptime</span><strong id="uptime">-</strong></div></section>
<section><h2>Camera</h2><img class="preview" id="preview" alt="Live camera preview"><form id="camera-form"><div class="grid">
<div><label for="resolution">Resolution</label><select id="resolution" name="resolution"><option value="8">QVGA (320x240)</option><option value="9">VGA (640x480)</option><option value="10">SVGA (800x600)</option><option value="11">XGA (1024x768)</option><option value="12">UXGA (1600x1200)</option></select></div>
<div><label for="frame-rate">Frame rate</label><select id="frame-rate" name="frame_rate"><option>5</option><option>10</option><option>15</option><option>20</option></select></div>
<label>JPEG quality <span class="muted">(10=best, 63=compressed)</span><div class="range"><input id="quality" name="quality" type="range" min="10" max="63"><output id="quality-value"></output></div></label>
<label>Brightness<div class="range"><input id="brightness" name="brightness" type="range" min="-2" max="2" step="1"><output id="brightness-value"></output></div></label>
<label>Contrast<div class="range"><input id="contrast" name="contrast" type="range" min="-2" max="2" step="1"><output id="contrast-value"></output></div></label>
<label>Saturation<div class="range"><input id="saturation" name="saturation" type="range" min="-2" max="2" step="1"><output id="saturation-value"></output></div></label>
<label class="check"><input id="vertical-flip" name="vertical_flip" type="checkbox">Vertical flip</label><label class="check"><input id="horizontal-mirror" name="horizontal_mirror" type="checkbox">Horizontal mirror</label>
</div><div class="actions"><button type="submit" class="btn-primary">Apply camera</button><p id="camera-message" class="message"></p></div></form></section>
<section class="admin-only"><h2>Device Settings</h2><div class="setting-link"><div><strong>WiFi configuration</strong><p class="muted" id="wifi-summary">Manage SSID and network address</p></div><button type="button" class="quiet" id="wifi-open">Configure</button></div><div class="setting-link"><div><strong>Account password</strong><p class="muted">Update the password for this account</p></div><button type="button" class="quiet" id="password-open">Change</button></div></section>
<div id="wifi-modal" class="modal" aria-hidden="true"><div class="modal-content" role="dialog" aria-modal="true" aria-labelledby="wifi-title"><div class="modal-header"><h2 id="wifi-title">WiFi settings</h2><button type="button" class="close-btn" data-close="wifi-modal" aria-label="Close">&times;</button></div><form id="network-form"><div class="grid">
<div class="full"><label for="ssid">WiFi SSID</label><input id="ssid" name="wifi_ssid" maxlength="31" required></div>
<div class="full"><label for="wifi-password">WiFi password</label><input id="wifi-password" name="wifi_password" type="password" maxlength="31" autocomplete="new-password"><p id="password-state" class="muted"></p></div>
<label class="check full"><input id="clear-password" type="checkbox">Clear stored password</label>
<label class="check full"><input id="dhcp" type="checkbox" checked>Use DHCP</label>
<div id="static-fields" class="full grid hidden"><div><label for="static-ip">Static IP</label><input id="static-ip" name="static_ip" inputmode="decimal"></div><div><label for="gateway">Gateway</label><input id="gateway" name="gateway" inputmode="decimal"></div><div><label for="subnet">Subnet</label><input id="subnet" name="subnet" inputmode="decimal"></div></div>
</div><div class="actions"><button type="button" class="quiet" data-close="wifi-modal">Cancel</button><button type="submit">Save network</button><p id="network-message" class="message"></p></div></form></div></div>
<div id="password-modal" class="modal" aria-hidden="true"><div class="modal-content" role="dialog" aria-modal="true" aria-labelledby="password-title"><div class="modal-header"><h2 id="password-title">Change password</h2><button type="button" class="close-btn" data-close="password-modal" aria-label="Close">&times;</button></div><form id="password-form">
<label for="current">Current password</label><input id="current" name="current_password" type="password" required>
<label for="new">New password</label><input id="new" name="new_password" type="password" minlength="4" maxlength="31" required>
<label for="confirm">Confirm new password</label><input id="confirm" name="confirm_password" type="password" minlength="4" maxlength="31" required>
<div class="actions"><button type="button" class="quiet" data-close="password-modal">Cancel</button><button type="submit">Save password</button></div><div id="message" class="message"></div></form></div></div>
<script>
const message=document.getElementById('message');
document.getElementById('preview').src='/stream?ts='+Date.now();
const openModal=id=>{const el=document.getElementById(id);el.classList.add('show');el.setAttribute('aria-hidden','false')};
const closeModal=id=>{const el=document.getElementById(id);el.classList.remove('show');el.setAttribute('aria-hidden','true');if(id==='password-modal'){message.textContent='';message.className='message';document.getElementById('password-form').reset()}if(id==='wifi-modal'){networkMessage.textContent='';networkMessage.className='message';document.getElementById('clear-password').checked=false;document.getElementById('wifi-password').value=''}};
document.getElementById('wifi-open').onclick=()=>openModal('wifi-modal');document.getElementById('password-open').onclick=()=>openModal('password-modal');
document.querySelectorAll('[data-close]').forEach(btn=>btn.onclick=()=>closeModal(btn.dataset.close));
document.querySelectorAll('.modal').forEach(modal=>modal.onclick=e=>{if(e.target===modal)closeModal(modal.id)});
document.addEventListener('keydown',e=>{if(e.key==='Escape')document.querySelectorAll('.modal.show').forEach(m=>closeModal(m.id))});
document.getElementById('logout').onclick=async()=>{await fetch('/api/logout',{method:'POST'});location.href='/'};
const api=async(url,options,keepUnauthorized=false)=>{const r=await fetch(url,options);if(r.status===401&&!keepUnauthorized){location.href='/';throw new Error('Unauthorized')}return r};
const dhcp=document.getElementById('dhcp'),staticFields=document.getElementById('static-fields'),networkMessage=document.getElementById('network-message');
const toggleStatic=()=>staticFields.classList.toggle('hidden',dhcp.checked);dhcp.onchange=toggleStatic;
const uptime=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+h+'h '+m+'m'};
async function load(){try{const tr=await api('/api/status');const t=await tr.json();const isAdmin=t.auth_level==='admin';document.getElementById('role').textContent=isAdmin?'Admin':'User';document.querySelectorAll('.admin-only').forEach(el=>el.classList.toggle('hidden',!isAdmin));
const s=isAdmin?await (await api('/api/settings')).json():{};const c=isAdmin?s:t;
if(isAdmin){document.getElementById('ssid').value=s.wifi_ssid||'';dhcp.checked=s.use_dhcp;document.getElementById('static-ip').value=s.static_ip||'';document.getElementById('gateway').value=s.gateway||'';document.getElementById('subnet').value=s.subnet||'';document.getElementById('password-state').textContent=s.wifi_password_set?'A password is stored. Leave blank to keep it.':'No password stored.';toggleStatic();}
document.getElementById('resolution').value=c.camera_resolution;document.getElementById('quality').value=c.camera_quality;document.getElementById('frame-rate').value=c.frame_rate;document.getElementById('brightness').value=c.brightness;document.getElementById('contrast').value=c.contrast;document.getElementById('saturation').value=c.saturation;document.getElementById('vertical-flip').checked=c.vertical_flip;document.getElementById('horizontal-mirror').checked=c.horizontal_mirror;['quality','brightness','contrast','saturation'].forEach(id=>document.getElementById(id+'-value').textContent=document.getElementById(id).value);
document.getElementById('wifi-state').textContent=t.wifi_connected?'Connected':'Disconnected';document.getElementById('ip-address').textContent=t.ip_address||'-';document.getElementById('uptime').textContent=uptime(t.uptime_seconds||0)}catch(e){networkMessage.className='message error';networkMessage.textContent='Unable to load settings'}}
const cameraMessage=document.getElementById('camera-message');
['quality','brightness','contrast','saturation'].forEach(id=>{const input=document.getElementById(id),output=document.getElementById(id+'-value');const update=()=>output.textContent=input.value;input.oninput=update;update()});
document.getElementById('camera-form').onsubmit=async(e)=>{e.preventDefault();const body=new URLSearchParams(new FormData(e.target));body.set('vertical_flip',String(document.getElementById('vertical-flip').checked));body.set('horizontal_mirror',String(document.getElementById('horizontal-mirror').checked));cameraMessage.className='message';cameraMessage.textContent='Applying...';try{const r=await api('/api/camera/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json();cameraMessage.className='message '+(r.ok?'ok':'error');cameraMessage.textContent=d.message||'Request failed'}catch(err){cameraMessage.className='message error';cameraMessage.textContent='Request failed'}};
document.getElementById('network-form').onsubmit=async(e)=>{e.preventDefault();const body=new URLSearchParams(new FormData(e.target));body.set('use_dhcp',String(dhcp.checked));body.set('clear_wifi_password',String(document.getElementById('clear-password').checked));networkMessage.className='message';networkMessage.textContent='Saving...';
try{const r=await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json();networkMessage.className='message '+(r.ok?'notice':'error');networkMessage.textContent=r.ok?(d.message+'. Expected address: '+d.expected_ip):(d.message||'Request failed')}catch(err){networkMessage.className='message error';networkMessage.textContent='Connection closed before confirmation'}};
document.getElementById('password-form').onsubmit=async(e)=>{e.preventDefault();const body=new URLSearchParams(new FormData(e.target));if(body.get('new_password')!==body.get('confirm_password')){message.className='message error';message.textContent='New passwords do not match';return}
const r=await api('/api/change-password',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body},true);const d=await r.json();
message.className='message '+(r.ok?'ok':'error');message.textContent=d.message||'Request failed';if(r.ok){e.target.reset();setTimeout(()=>closeModal('password-modal'),900)}};
load();
</script></main></body></html>)HTML";

} // namespace CameraHtml

#endif // HTML_PAGES_H
