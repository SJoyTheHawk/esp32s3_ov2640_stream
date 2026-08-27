"""
ESP32-S3 摄像头视频流服务器

功能：
  - 接收 ESP32 推送的 JPEG 帧
  - Web 控制页面：实时预览、拍照、录像
  - 远程控制 ESP32：拍照反馈、分辨率切换
  - 合成视频文件 (AVI)

使用方法：
  pip install flask opencv-python numpy
  python server.py
  浏览器打开 http://localhost:5000
"""

import os
import time
import json
import queue
import shutil
import threading
from datetime import datetime

import cv2
import numpy as np
import requests
from flask import Flask, Response, request, jsonify

# ==================== 配置 ====================
PHOTO_DIR = os.path.join(os.path.dirname(__file__), "photos")
PHOTO_DIR2 = os.path.join(os.path.dirname(__file__), "photos2")
VIDEO_DIR = os.path.join(os.path.dirname(__file__), "videos")
VIDEO_DIR2 = os.path.join(os.path.dirname(__file__), "videos2")
UPLOAD_URL = "http://8.217.234.14:8888/upload"
# VIDEO_FPS = 20
# FRAME_W, FRAME_H = 800, 600
VIDEO_FPS = 20
FRAME_W, FRAME_H = 1600, 1200

# 支持的分辨率
RESOLUTIONS = {
    "QVGA": {"width": 320, "height": 240, "framesize": 8},
    "VGA":  {"width": 640, "height": 480, "framesize": 9},
    "SVGA": {"width": 800, "height": 600, "framesize": 10},
    "XGA":  {"width": 1024, "height": 768, "framesize": 11},
    "UXGA": {"width": 1600, "height": 1200, "framesize": 12},
}

os.makedirs(PHOTO_DIR, exist_ok=True)
os.makedirs(PHOTO_DIR2, exist_ok=True)
os.makedirs(VIDEO_DIR, exist_ok=True)
os.makedirs(VIDEO_DIR2, exist_ok=True)

app = Flask(__name__)

# ==================== 状态 ====================
lock = threading.Lock()

stream_active = False
last_frame_time = 0.0
last_frame = None

is_recording = False
recording_thread = None
video_frame_count = 0
record_start_time = 0.0

photo_count = len([f for f in os.listdir(PHOTO_DIR) if f.endswith((".jpg", ".jpeg"))])

# 帧队列：ESP32 -> 录像线程
frame_queue = queue.Queue(maxsize=300)

# 命令队列：Web UI -> ESP32
command_queue = queue.Queue(maxsize=10)

# 当前配置
current_config = {
    "resolution": "SVGA",
    "quality": 12,
}


# ==================== 帧接收 ====================
@app.route("/stream", methods=["POST"])
def receive_frame():
    """接收 ESP32 推送的 JPEG 帧，返回 JSON 响应（可携带命令）"""
    global stream_active, last_frame_time, last_frame

    jpeg_data = request.get_data()
    if not jpeg_data:
        return jsonify({"ok": False, "error": "empty"}), 400

    now = time.time()
    with lock:
        stream_active = True
        last_frame_time = now
        last_frame = jpeg_data

    # 如果正在录像，把帧放入队列
    if is_recording:
        try:
            frame_queue.put_nowait(jpeg_data)
        except queue.Full:
            pass  # 丢弃，不阻塞 ESP32

    # 检查是否有待发送的命令
    try:
        cmd = command_queue.get_nowait()
        return jsonify(cmd), 200
    except queue.Empty:
        return jsonify({"ok": True}), 200


@app.route("/status")
def status():
    """服务器状态 API"""
    with lock:
        elapsed = time.time() - record_start_time if is_recording else 0
        alive = (time.time() - last_frame_time) < 3 if last_frame_time else False
        return jsonify({
            "stream_active": alive,
            "is_recording": is_recording,
            "photo_count": photo_count,
            "record_seconds": round(elapsed, 1),
            "config": current_config,
        })


# ==================== 控制 API ====================
@app.route("/api/photo", methods=["POST"])
def take_photo():
    """拍照 - 同时通知 ESP32 闪烁 LED"""
    global photo_count
    with lock:
        if last_frame is None:
            return jsonify({"ok": False, "error": "no frame available"})
        photo_count += 1
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        path = os.path.join(PHOTO_DIR, f"photo_{ts}.jpg")
        with open(path, "wb") as f:
            f.write(last_frame)

    # 发送命令到 ESP32
    try:
        command_queue.put_nowait({"cmd": "photo"})
    except queue.Full:
        pass

    return jsonify({"ok": True, "path": path})


@app.route("/api/video/start", methods=["POST"])
def start_video():
    """开始录像"""
    global is_recording, recording_thread, video_frame_count, record_start_time

    if is_recording:
        return jsonify({"ok": False, "error": "already recording"})

    with lock:
        is_recording = True
        video_frame_count = 0
        record_start_time = time.time()
        # 清空队列
        while not frame_queue.empty():
            try:
                frame_queue.get_nowait()
            except queue.Empty:
                break

    recording_thread = threading.Thread(target=_record_worker, daemon=True)
    recording_thread.start()
    print(f"[VIDEO] Recording started")
    return jsonify({"ok": True})


@app.route("/api/video/stop", methods=["POST"])
def stop_video():
    """停止录像"""
    global is_recording

    if not is_recording:
        return jsonify({"ok": False, "error": "not recording"})

    with lock:
        is_recording = False

    if recording_thread:
        recording_thread.join(timeout=10)

    print(f"[VIDEO] Recording stopped. Frames: {video_frame_count}")
    return jsonify({"ok": True, "frames": video_frame_count})


@app.route("/api/resolution", methods=["POST"])
def set_resolution():
    """切换分辨率和质量"""
    data = request.get_json()
    if not data:
        return jsonify({"ok": False, "error": "no data"})

    resolution = data.get("resolution", current_config["resolution"])
    quality = data.get("quality", current_config["quality"])

    # 验证参数
    if resolution not in RESOLUTIONS:
        return jsonify({"ok": False, "error": f"unknown resolution: {resolution}"})
    if not isinstance(quality, int) or quality < 10 or quality > 63:
        return jsonify({"ok": False, "error": "quality must be 10-63"})

    # 更新配置
    with lock:
        current_config["resolution"] = resolution
        current_config["quality"] = quality

    # 发送命令到 ESP32
    cmd = {
        "cmd": "set_resolution",
        "value": resolution,
        "quality": quality,
    }
    try:
        command_queue.put_nowait(cmd)
    except queue.Full:
        return jsonify({"ok": False, "error": "command queue full"})

    print(f"[CONFIG] Resolution: {resolution}, Quality: {quality}")
    return jsonify({"ok": True, "config": current_config})


@app.route("/api/config")
def get_config():
    """获取当前配置"""
    return jsonify(current_config)


# ==================== 上传 API ====================
@app.route("/api/upload", methods=["POST"])
def upload_photos():
    """上传 photos/ 下所有照片到远程服务器，成功后移到 photos2/"""
    photos = [f for f in os.listdir(PHOTO_DIR) if f.endswith((".jpg", ".jpeg"))]
    if not photos:
        return jsonify({"ok": False, "error": "no photos to upload"})

    results = {"success": [], "failed": []}
    for fname in photos:
        fpath = os.path.join(PHOTO_DIR, fname)
        try:
            with open(fpath, "rb") as f:
                resp = requests.post(UPLOAD_URL, files={"file": (fname, f, "image/jpeg")}, timeout=30)
            if resp.status_code < 300:
                shutil.move(fpath, os.path.join(PHOTO_DIR2, fname))
                results["success"].append(fname)
            else:
                results["failed"].append({"file": fname, "status": resp.status_code})
        except Exception as e:
            results["failed"].append({"file": fname, "error": str(e)})

    print(f"[UPLOAD] {len(results['success'])} ok, {len(results['failed'])} failed")
    return jsonify({"ok": True, **results})


@app.route("/api/upload_videos", methods=["POST"])
def upload_videos():
    """上传 videos/ 下所有视频到远程服务器，成功后移到 videos2/"""
    videos = [f for f in os.listdir(VIDEO_DIR) if f.endswith((".avi", ".mp4", ".mov"))]
    if not videos:
        return jsonify({"ok": False, "error": "no videos to upload"})

    results = {"success": [], "failed": []}
    for fname in videos:
        fpath = os.path.join(VIDEO_DIR, fname)
        try:
            with open(fpath, "rb") as f:
                resp = requests.post(UPLOAD_URL, files={"file": (fname, f, "video/avi")}, timeout=120)
            if resp.status_code < 300:
                shutil.move(fpath, os.path.join(VIDEO_DIR2, fname))
                results["success"].append(fname)
            else:
                results["failed"].append({"file": fname, "status": resp.status_code})
        except Exception as e:
            results["failed"].append({"file": fname, "error": str(e)})

    print(f"[UPLOAD-VIDEO] {len(results['success'])} ok, {len(results['failed'])} failed")
    return jsonify({"ok": True, **results})


def _record_worker():
    """录像工作线程：从队列取帧写入 AVI"""
    global video_frame_count

    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    path = os.path.join(VIDEO_DIR, f"video_{ts}.avi")
    fourcc = cv2.VideoWriter_fourcc(*"MJPG")
    out = cv2.VideoWriter(path, fourcc, VIDEO_FPS, (FRAME_W, FRAME_H))

    print(f"[VIDEO] Writing to {path}")

    idle_count = 0
    while True:
        # 检查是否应该停止
        if not is_recording and frame_queue.empty():
            break

        try:
            jpeg_data = frame_queue.get(timeout=3)
        except queue.Empty:
            if not is_recording and frame_queue.empty():
                break
            idle_count += 1
            if idle_count > 10:
                print("[VIDEO] Stream lost, stopping recording")
                break
            continue

        idle_count = 0
        # JPEG -> numpy array -> OpenCV frame
        nparr = np.frombuffer(jpeg_data, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        if frame is not None:
            if frame.shape[1] != FRAME_W or frame.shape[0] != FRAME_H:
                frame = cv2.resize(frame, (FRAME_W, FRAME_H))
            out.write(frame)
            video_frame_count += 1

    out.release()
    duration = time.time() - record_start_time
    print(f"[VIDEO] Saved: {path} | {video_frame_count} frames | {duration:.1f}s")


# ==================== Web 页面 ====================
@app.route("/")
def index():
    return """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32-S3 Camera Control</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: #0f172a; color: #e2e8f0;
    min-height: 100vh; display: flex; flex-direction: column; align-items: center;
  }
  header {
    width: 100%; padding: 16px 24px;
    background: #1e293b; border-bottom: 1px solid #334155;
    display: flex; justify-content: space-between; align-items: center;
  }
  header h1 { font-size: 20px; font-weight: 600; }
  .status-badge {
    padding: 4px 12px; border-radius: 20px; font-size: 13px; font-weight: 500;
  }
  .status-badge.on  { background: #065f46; color: #6ee7b7; }
  .status-badge.off { background: #7f1d1d; color: #fca5a5; }

  .main { width: 100%; max-width: 900px; padding: 24px; flex: 1; }

  .video-box {
    background: #1e293b; border-radius: 12px; overflow: hidden;
    border: 1px solid #334155; margin-bottom: 20px;
    position: relative;
  }
  .video-box img {
    width: 100%; display: block; background: #000;
    min-height: 200px; object-fit: contain;
  }
  .video-label {
    padding: 10px 16px; font-size: 13px; color: #94a3b8;
    border-top: 1px solid #334155;
    display: flex; justify-content: space-between;
  }

  /* 拍照闪光动画 */
  .flash-overlay {
    position: absolute; top: 0; left: 0; right: 0; bottom: 0;
    background: white; opacity: 0; pointer-events: none;
    transition: opacity 0.1s;
  }
  .flash-overlay.active { opacity: 0.8; }

  .controls {
    display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 12px; margin-bottom: 20px;
  }
  .btn {
    padding: 14px 20px; border: none; border-radius: 10px;
    font-size: 15px; font-weight: 600; cursor: pointer;
    transition: all 0.2s; display: flex; align-items: center;
    justify-content: center; gap: 8px;
  }
  .btn:hover { transform: translateY(-1px); filter: brightness(1.1); }
  .btn:active { transform: translateY(0); }
  .btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }

  .btn-photo  { background: #2563eb; color: white; }
  .btn-rec    { background: #dc2626; color: white; }
  .btn-rec.active { background: #991b1b; animation: pulse 1.5s infinite; }
  .btn-upload { background: #7c3aed; color: white; }
  .btn-upload-vid { background: #6d28d9; color: white; }
  .btn-status { background: #334155; color: #94a3b8; cursor: default; }

  @keyframes pulse {
    0%, 100% { box-shadow: 0 0 0 0 rgba(220, 38, 38, 0.4); }
    50%      { box-shadow: 0 0 0 8px rgba(220, 38, 38, 0); }
  }

  .config-panel {
    background: #1e293b; border-radius: 12px; padding: 16px;
    border: 1px solid #334155; margin-bottom: 20px;
  }
  .config-panel h3 {
    font-size: 14px; color: #94a3b8; margin-bottom: 12px;
    text-transform: uppercase; letter-spacing: 0.5px;
  }
  .config-row {
    display: flex; gap: 16px; align-items: center; flex-wrap: wrap;
  }
  .config-item {
    display: flex; flex-direction: column; gap: 4px;
  }
  .config-item label {
    font-size: 12px; color: #64748b;
  }
  .config-item select, .config-item input[type="range"] {
    background: #0f172a; border: 1px solid #334155; border-radius: 6px;
    color: #e2e8f0; padding: 8px 12px; font-size: 14px;
  }
  .config-item select { min-width: 120px; }
  .config-item input[type="range"] { width: 150px; }
  .quality-value { font-size: 13px; color: #60a5fa; min-width: 30px; }
  .btn-apply {
    background: #059669; color: white; padding: 8px 20px;
    border: none; border-radius: 6px; font-size: 14px; font-weight: 500;
    cursor: pointer; margin-left: auto;
  }
  .btn-apply:hover { background: #047857; }

  .info-cards {
    display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px;
  }
  .card {
    background: #1e293b; border-radius: 10px; padding: 16px;
    border: 1px solid #334155; text-align: center;
  }
  .card-value { font-size: 28px; font-weight: 700; margin-bottom: 4px; }
  .card-label { font-size: 12px; color: #64748b; text-transform: uppercase; letter-spacing: 0.5px; }
</style>
</head>
<body>

<header>
  <h1>ESP32-S3 Camera</h1>
  <span id="streamBadge" class="status-badge off">OFFLINE</span>
</header>

<div class="main">
  <div class="video-box">
    <div id="flashOverlay" class="flash-overlay"></div>
    <img id="liveView" src="/video_feed" alt="Live Stream">
    <div class="video-label">
      <span>Live Preview</span>
      <span id="fpsDisplay">-- fps</span>
    </div>
  </div>

  <div class="config-panel">
    <h3>Camera Settings</h3>
    <div class="config-row">
      <div class="config-item">
        <label>Resolution</label>
        <select id="resolutionSelect">
          <option value="QVGA">QVGA (320x240)</option>
          <option value="VGA">VGA (640x480)</option>
          <option value="SVGA" selected>SVGA (800x600)</option>
          <option value="XGA">XGA (1024x768)</option>
          <option value="UXGA">UXGA (1600x1200)</option>
        </select>
      </div>
      <div class="config-item">
        <label>JPEG Quality</label>
        <div style="display:flex;align-items:center;gap:8px;">
          <input type="range" id="qualitySlider" min="10" max="30" value="12">
          <span id="qualityValue" class="quality-value">12</span>
        </div>
      </div>
      <button class="btn-apply" onclick="applyConfig()">Apply</button>
    </div>
  </div>

  <div class="controls">
    <button class="btn btn-photo" onclick="takePhoto()">Take Photo</button>
    <button id="recBtn" class="btn btn-rec" onclick="toggleRecord()">Start Recording</button>
    <button id="uploadBtn" class="btn btn-upload" onclick="uploadPhotos()">Upload Photos</button>
    <button id="uploadVidBtn" class="btn btn-upload-vid" onclick="uploadVideos()">Upload Videos</button>
  </div>

  <div class="info-cards">
    <div class="card">
      <div class="card-value" id="photoCount">0</div>
      <div class="card-label">Photos</div>
    </div>
    <div class="card">
      <div class="card-value" id="recTime">0s</div>
      <div class="card-label">Record Time</div>
    </div>
    <div class="card">
      <div class="card-value" id="recState">IDLE</div>
      <div class="card-label">Status</div>
    </div>
  </div>
</div>

<script>
const RES_LABELS = {
  QVGA: 'QVGA (320x240)',
  VGA: 'VGA (640x480)',
  SVGA: 'SVGA (800x600)',
  XGA: 'XGA (1024x768)',
  UXGA: 'UXGA (1600x1200)'
};

let recording = false;
let frameCount = 0;
let lastCheck = Date.now();
let userEditing = false;

function updateResLabel() {
  const sel = document.getElementById('resolutionSelect').value;
  document.getElementById('resolutionLabel').textContent = RES_LABELS[sel] || sel;
}

// 用户操作表单时标记为编辑中，防止轮询覆盖
document.getElementById('qualitySlider').addEventListener('input', (e) => {
  document.getElementById('qualityValue').textContent = e.target.value;
  userEditing = true;
});
document.getElementById('resolutionSelect').addEventListener('change', () => {
  userEditing = true;
  updateResLabel();
});

// FPS 计算
document.getElementById('liveView').addEventListener('load', () => {
  frameCount++;
});

setInterval(() => {
  const now = Date.now();
  const elapsed = (now - lastCheck) / 1000;
  const fps = Math.round(frameCount / elapsed);
  document.getElementById('fpsDisplay').textContent = fps + ' fps';
  frameCount = 0;
  lastCheck = now;
}, 1000);

// 状态轮询
setInterval(async () => {
  try {
    const r = await fetch('/status');
    const d = await r.json();
    const badge = document.getElementById('streamBadge');
    badge.textContent = d.stream_active ? 'LIVE' : 'OFFLINE';
    badge.className = 'status-badge ' + (d.stream_active ? 'on' : 'off');
    document.getElementById('photoCount').textContent = d.photo_count;
    document.getElementById('recTime').textContent = d.record_seconds + 's';
    document.getElementById('recState').textContent = d.is_recording ? 'REC' : 'IDLE';

    // 只在首次或 Apply 成功后同步配置，不覆盖用户正在编辑的值
    if (d.config && !userEditing) {
      document.getElementById('resolutionSelect').value = d.config.resolution;
      document.getElementById('qualitySlider').value = d.config.quality;
      document.getElementById('qualityValue').textContent = d.config.quality;
      updateResLabel();
    }
  } catch (e) {}
}, 1000);

// 拍照 + 闪光动画
async function takePhoto() {
  const flash = document.getElementById('flashOverlay');
  flash.classList.add('active');

  try {
    const r = await fetch('/api/photo', {method:'POST'});
    const d = await r.json();
    setTimeout(() => flash.classList.remove('active'), 150);
    if (d.ok) {
      // 成功提示（不用 alert，用更优雅的方式）
      const btn = document.querySelector('.btn-photo');
      const orig = btn.innerHTML;
      btn.innerHTML = '✓ Saved!';
      btn.style.background = '#059669';
      setTimeout(() => { btn.innerHTML = orig; btn.style.background = ''; }, 1500);
    } else {
      alert('Failed: ' + (d.error || 'unknown'));
    }
  } catch (e) {
    flash.classList.remove('active');
    alert('Error: ' + e.message);
  }
}

// 录像切换
async function toggleRecord() {
  const btn = document.getElementById('recBtn');
  if (!recording) {
    const r = await fetch('/api/video/start', {method:'POST'});
    const d = await r.json();
    if (d.ok) {
      recording = true;
      btn.textContent = 'Stop Recording';
      btn.classList.add('active');
    } else {
      alert(d.error);
    }
  } else {
    const r = await fetch('/api/video/stop', {method:'POST'});
    const d = await r.json();
    if (d.ok) {
      recording = false;
      btn.textContent = 'Start Recording';
      btn.classList.remove('active');
      alert('Video saved! ' + d.frames + ' frames');
    }
  }
}

// 应用分辨率/质量设置
async function applyConfig() {
  const resolution = document.getElementById('resolutionSelect').value;
  const quality = parseInt(document.getElementById('qualitySlider').value);

  const btn = document.querySelector('.btn-apply');
  btn.textContent = 'Applying...';
  btn.disabled = true;

  try {
    const r = await fetch('/api/resolution', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({resolution, quality})
    });
    const d = await r.json();
    if (d.ok) {
      userEditing = false;
      btn.textContent = '✓ Done';
      setTimeout(() => { btn.textContent = 'Apply'; btn.disabled = false; }, 1500);
    } else {
      alert('Failed: ' + (d.error || 'unknown'));
      btn.textContent = 'Apply';
      btn.disabled = false;
    }
  } catch (e) {
    alert('Error: ' + e.message);
    btn.textContent = 'Apply';
    btn.disabled = false;
  }
}

// 上传照片到远程服务器
async function uploadPhotos() {
  const btn = document.getElementById('uploadBtn');
  btn.textContent = 'Uploading...';
  btn.disabled = true;

  try {
    const r = await fetch('/api/upload', {method: 'POST'});
    const d = await r.json();
    if (d.ok) {
      const msg = `Uploaded: ${d.success.length} | Failed: ${d.failed.length}`;
      btn.innerHTML = '&check; Done';
      btn.style.background = '#059669';
      setTimeout(() => { btn.textContent = 'Upload Photos'; btn.style.background = ''; btn.disabled = false; }, 2000);
      if (d.failed.length) alert(msg + '\\n' + d.failed.map(f => f.file || f.error).join('\\n'));
    } else {
      alert(d.error || 'No photos');
      btn.textContent = 'Upload Photos';
      btn.disabled = false;
    }
  } catch (e) {
    alert('Error: ' + e.message);
    btn.textContent = 'Upload Photos';
    btn.disabled = false;
  }
}

// 上传视频到远程服务器
async function uploadVideos() {
  const btn = document.getElementById('uploadVidBtn');
  btn.textContent = 'Uploading...';
  btn.disabled = true;

  try {
    const r = await fetch('/api/upload_videos', {method: 'POST'});
    const d = await r.json();
    if (d.ok) {
      const msg = `Uploaded: ${d.success.length} | Failed: ${d.failed.length}`;
      btn.innerHTML = '&check; Done';
      btn.style.background = '#059669';
      setTimeout(() => { btn.textContent = 'Upload Videos'; btn.style.background = ''; btn.disabled = false; }, 2000);
      if (d.failed.length) alert(msg + '\\n' + d.failed.map(f => f.file || f.error).join('\\n'));
    } else {
      alert(d.error || 'No videos');
      btn.textContent = 'Upload Videos';
      btn.disabled = false;
    }
  } catch (e) {
    alert('Error: ' + e.message);
    btn.textContent = 'Upload Videos';
    btn.disabled = false;
  }
}
</script>

</body>
</html>"""


# ==================== 视频流 ====================
@app.route("/video_feed")
def video_feed():
    """MJPEG 实时预览流（给浏览器 <img> 标签用）"""
    def generate():
        while True:
            with lock:
                frame = last_frame
                active = stream_active and (time.time() - last_frame_time) < 3
            if active and frame:
                yield (b"--frame\r\n"
                       b"Content-Type: image/jpeg\r\n\r\n"
                       + frame + b"\r\n")
            else:
                # 无信号占位图
                placeholder = _no_signal_image()
                yield (b"--frame\r\n"
                       b"Content-Type: image/jpeg\r\n\r\n"
                       + placeholder + b"\r\n")
            time.sleep(0.05)  # ~20fps

    return Response(generate(), mimetype="multipart/x-mixed-replace;boundary=frame")


def _no_signal_image():
    """生成无信号占位图"""
    img = np.zeros((FRAME_H, FRAME_W, 3), dtype=np.uint8)
    text = "NO SIGNAL"
    font = cv2.FONT_HERSHEY_SIMPLEX
    (tw, th), _ = cv2.getTextSize(text, font, 1.5, 3)
    x = (FRAME_W - tw) // 2
    y = (FRAME_H + th) // 2
    cv2.putText(img, text, (x, y), font, 1.5, (60, 60, 60), 3)
    _, jpeg = cv2.imencode(".jpg", img, [cv2.IMWRITE_JPEG_QUALITY, 50])
    return jpeg.tobytes()


# ==================== 启动 ====================
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--host", type=str, default="0.0.0.0")
    args = parser.parse_args()

    print("=" * 40)
    print("  ESP32 Camera Server")
    print(f"  http://localhost:{args.port}")
    print("=" * 40)
    print(f"  Photos: {PHOTO_DIR}")
    print(f"  Uploaded: {PHOTO_DIR2}")
    print(f"  Videos: {VIDEO_DIR}")
    print(f"  Uploaded Videos: {VIDEO_DIR2}")
    print(f"  Upload URL: {UPLOAD_URL}")
    print("=" * 40)

    app.run(host=args.host, port=args.port, threaded=True)
