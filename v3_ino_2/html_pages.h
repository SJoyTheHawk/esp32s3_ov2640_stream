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
body{font-family:system-ui,sans-serif;background:#0b1220;color:#e5e7eb;margin:0;padding:24px}
main{max-width:720px;margin:auto}header{display:flex;justify-content:space-between;align-items:center;gap:16px;border-bottom:1px solid #334155;padding-bottom:16px}
h1{font-size:1.35rem;margin:0}section{margin-top:24px;padding:20px;background:#172033;border:1px solid #334155;border-radius:8px}
button{padding:10px 14px;border:0;border-radius:5px;background:#38bdf8;color:#082f49;font-weight:700;cursor:pointer}.danger{background:#f87171;color:#450a0a}
label{display:block;margin:14px 0 6px;color:#94a3b8;font-size:.9rem}input{width:100%;box-sizing:border-box;padding:10px;background:#0f172a;color:#e5e7eb;border:1px solid #475569;border-radius:5px}
.message{min-height:1.3em;margin-top:12px}.error{color:#fca5a5}.ok{color:#86efac}
</style></head>
<body><main><header><h1>ESP32-S3 Camera</h1><button id="logout" class="danger">Log out</button></header>
<section><strong>Authentication active</strong><p>Camera settings and network configuration will be available through the settings API.</p></section>
<section><h2>Change password</h2><form id="password-form">
<label for="current">Current password</label><input id="current" name="current_password" type="password" required>
<label for="new">New password</label><input id="new" name="new_password" type="password" minlength="4" maxlength="31" required>
<button type="submit">Save password</button><div id="message" class="message"></div></form></section>
<script>
const message=document.getElementById('message');
document.getElementById('logout').onclick=async()=>{await fetch('/api/logout',{method:'POST'});location.href='/'};
document.getElementById('password-form').onsubmit=async(e)=>{e.preventDefault();const body=new URLSearchParams(new FormData(e.target));
const r=await fetch('/api/change-password',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json();
message.className='message '+(r.ok?'ok':'error');message.textContent=d.message||'Request failed';if(r.ok)e.target.reset()};
</script></main></body></html>)HTML";

} // namespace CameraHtml

#endif // HTML_PAGES_H
