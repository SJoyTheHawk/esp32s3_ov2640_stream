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
*{box-sizing:border-box}body{font-family:system-ui,sans-serif;background:#111315;color:#f3f4f6;margin:0}main{max-width:820px;margin:auto;padding:24px}
header{display:flex;justify-content:space-between;align-items:center;gap:16px;border-bottom:1px solid #3f454b;padding:4px 0 18px}h1{font-size:1.35rem;margin:0}h2{font-size:1.05rem;margin:0 0 18px}
section{padding:24px 0;border-bottom:1px solid #34393e}.status{display:grid;grid-template-columns:repeat(3,1fr);gap:16px}.metric span{display:block;color:#9da5ad;font-size:.78rem;text-transform:uppercase}.metric strong{display:block;margin-top:5px;overflow-wrap:anywhere}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:0 18px}.full{grid-column:1/-1}label{display:block;margin:14px 0 6px;color:#b2bac2;font-size:.88rem}input{width:100%;padding:10px;background:#1c2024;color:#f3f4f6;border:1px solid #4a5158;border-radius:5px;font:inherit}
.check{display:flex;align-items:center;gap:9px;color:#e5e7eb}.check input{width:18px;height:18px;margin:0}.actions{display:flex;align-items:center;gap:14px;margin-top:20px;flex-wrap:wrap}button{padding:10px 14px;border:0;border-radius:5px;background:#5eead4;color:#103b36;font-weight:700;cursor:pointer}.quiet{background:#343a40;color:#f3f4f6}.danger{background:#ef4444;color:#fff}
.message{min-height:1.3em;margin:0}.error{color:#fca5a5}.ok{color:#86efac}.notice{color:#fcd34d}.muted{color:#9da5ad;font-size:.85rem;margin:7px 0 0}.hidden{display:none}@media(max-width:600px){main{padding:18px}.grid,.status{grid-template-columns:1fr}.full{grid-column:auto}}
</style></head>
<body><main><header><h1>ESP32-S3 Camera</h1><button id="logout" class="danger">Log out</button></header>
<section class="status"><div class="metric"><span>WiFi</span><strong id="wifi-state">Loading</strong></div><div class="metric"><span>IP address</span><strong id="ip-address">-</strong></div><div class="metric"><span>Uptime</span><strong id="uptime">-</strong></div></section>
<section><h2>Network</h2><form id="network-form"><div class="grid">
<div class="full"><label for="ssid">WiFi SSID</label><input id="ssid" name="wifi_ssid" maxlength="31" required></div>
<div class="full"><label for="wifi-password">WiFi password</label><input id="wifi-password" name="wifi_password" type="password" maxlength="31" autocomplete="new-password"><p id="password-state" class="muted"></p></div>
<label class="check full"><input id="clear-password" type="checkbox">Clear stored password</label>
<label class="check full"><input id="dhcp" type="checkbox" checked>Use DHCP</label>
<div id="static-fields" class="full grid hidden"><div><label for="static-ip">Static IP</label><input id="static-ip" name="static_ip" inputmode="decimal"></div><div><label for="gateway">Gateway</label><input id="gateway" name="gateway" inputmode="decimal"></div><div><label for="subnet">Subnet</label><input id="subnet" name="subnet" inputmode="decimal"></div></div>
</div><div class="actions"><button type="submit">Save network</button><p id="network-message" class="message"></p></div></form></section>
<section><h2>Change password</h2><form id="password-form">
<label for="current">Current password</label><input id="current" name="current_password" type="password" required>
<label for="new">New password</label><input id="new" name="new_password" type="password" minlength="4" maxlength="31" required>
<button type="submit">Save password</button><div id="message" class="message"></div></form></section>
<script>
const message=document.getElementById('message');
document.getElementById('logout').onclick=async()=>{await fetch('/api/logout',{method:'POST'});location.href='/'};
const api=async(url,options)=>{const r=await fetch(url,options);if(r.status===401){location.href='/';throw new Error('Unauthorized')}return r};
const dhcp=document.getElementById('dhcp'),staticFields=document.getElementById('static-fields'),networkMessage=document.getElementById('network-message');
const toggleStatic=()=>staticFields.classList.toggle('hidden',dhcp.checked);dhcp.onchange=toggleStatic;
const uptime=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+h+'h '+m+'m'};
async function load(){try{const [sr,tr]=await Promise.all([api('/api/settings'),api('/api/status')]);const s=await sr.json(),t=await tr.json();
document.getElementById('ssid').value=s.wifi_ssid||'';dhcp.checked=s.use_dhcp;document.getElementById('static-ip').value=s.static_ip||'';document.getElementById('gateway').value=s.gateway||'';document.getElementById('subnet').value=s.subnet||'';document.getElementById('password-state').textContent=s.wifi_password_set?'A password is stored. Leave blank to keep it.':'No password stored.';toggleStatic();
document.getElementById('wifi-state').textContent=t.wifi_connected?'Connected':'Disconnected';document.getElementById('ip-address').textContent=t.ip_address||'-';document.getElementById('uptime').textContent=uptime(t.uptime_seconds||0)}catch(e){networkMessage.className='message error';networkMessage.textContent='Unable to load settings'}}
document.getElementById('network-form').onsubmit=async(e)=>{e.preventDefault();const body=new URLSearchParams(new FormData(e.target));body.set('use_dhcp',String(dhcp.checked));body.set('clear_wifi_password',String(document.getElementById('clear-password').checked));networkMessage.className='message';networkMessage.textContent='Saving...';
try{const r=await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json();networkMessage.className='message '+(r.ok?'notice':'error');networkMessage.textContent=r.ok?(d.message+'. Expected address: '+d.expected_ip):(d.message||'Request failed')}catch(err){networkMessage.className='message error';networkMessage.textContent='Connection closed before confirmation'}};
document.getElementById('password-form').onsubmit=async(e)=>{e.preventDefault();const body=new URLSearchParams(new FormData(e.target));
const r=await api('/api/change-password',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json();
message.className='message '+(r.ok?'ok':'error');message.textContent=d.message||'Request failed';if(r.ok)e.target.reset()};
load();
</script></main></body></html>)HTML";

} // namespace CameraHtml

#endif // HTML_PAGES_H
