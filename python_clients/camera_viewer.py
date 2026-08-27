#!/usr/bin/env python3
"""PyQt6 ESP32 MJPEG viewer with one-frame-per-second capture."""
import json, os, sys, threading, time
from datetime import datetime
from pathlib import Path
from queue import Empty, Queue
import cv2
from mjpeg_stream import frames
from PyQt6.QtCore import QEvent, QTimer, Qt
from PyQt6.QtGui import QImage, QPixmap
from PyQt6.QtWidgets import QApplication, QDialog, QDialogButtonBox, QFileDialog, QHBoxLayout, QLabel, QLineEdit, QMainWindow, QMessageBox, QPushButton, QVBoxLayout, QWidget

HERE = Path(__file__).resolve().parent
START_STYLE = "QPushButton { background-color: #188038; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-weight: 600; } QPushButton:hover { background-color: #147332; } QPushButton:pressed { background-color: #105b28; }"
STOP_STYLE = "QPushButton { background-color: #c5221f; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-weight: 600; } QPushButton:hover { background-color: #b01d1a; } QPushButton:pressed { background-color: #8e1715; }"
DEFAULTS = {"stream_url":"http://192.168.2.100/stream", "username":os.getenv("ESP32_CAMERA_USERNAME", "admin"), "password":os.getenv("ESP32_CAMERA_PASSWORD", ""), "capture_dir":"./captures", "auto_capture_enabled":True, "capture_interval":1.0, "last_window_size":[900,760]}

class Settings:
    def __init__(self, path=HERE / "settings.json"):
        self.path=Path(path)
        try: self.data={**DEFAULTS, **json.loads(self.path.read_text())}
        except (OSError, ValueError): self.data=DEFAULTS.copy()
    def save(self): self.path.write_text(json.dumps(self.data, indent=2) + "\n")

class ConnectDialog(QDialog):
    def __init__(self, default, username="admin", password=""):
        super().__init__(); self.setWindowTitle("Connect to ESP32"); self.remaining=10
        layout=QVBoxLayout(self); layout.addWidget(QLabel("MJPEG stream URL")); self.url=QLineEdit(default); layout.addWidget(self.url)
        layout.addWidget(QLabel("Username")); self.username=QLineEdit(username); layout.addWidget(self.username)
        layout.addWidget(QLabel("Password")); self.password=QLineEdit(password); self.password.setEchoMode(QLineEdit.EchoMode.Password); layout.addWidget(self.password)
        self.message=QLabel(); layout.addWidget(self.message); buttons=QDialogButtonBox(QDialogButtonBox.StandardButton.Cancel|QDialogButtonBox.StandardButton.Ok); self.ok=buttons.button(QDialogButtonBox.StandardButton.Ok); buttons.accepted.connect(self.accept); buttons.rejected.connect(self.reject); layout.addWidget(buttons)
        self.countdown_active=True; self.timer=QTimer(self); self.timer.timeout.connect(self.tick); QApplication.instance().installEventFilter(self); self.timer.start(1000); self.tick()
    def eventFilter(self, watched, event):
        if self.isVisible() and self.countdown_active and event.type() in (QEvent.Type.MouseButtonPress, QEvent.Type.KeyPress):
            self.cancel_countdown()
        return super().eventFilter(watched, event)
    def cancel_countdown(self):
        if self.countdown_active:
            self.countdown_active=False; self.timer.stop(); self.ok.setText("Connect")
    def tick(self):
        if not self.countdown_active: return
        self.ok.setText(f"Connect ({self.remaining})"); self.remaining-=1
        if self.remaining < 0: self.timer.stop(); self.accept()
    def accept(self):
        self.cancel_countdown(); QApplication.instance().removeEventFilter(self)
        if not self.url.text().strip(): self.message.setText("Enter a stream URL"); return
        super().accept()
    def reject(self):
        self.cancel_countdown(); QApplication.instance().removeEventFilter(self); super().reject()

class Viewer(QMainWindow):
    def __init__(self, settings):
        super().__init__(); self.settings=settings; self.stop=threading.Event(); self.frames=Queue(maxsize=1); self.saves=Queue(maxsize=2); self.notifications=Queue(maxsize=4); self.enabled=settings.data["auto_capture_enabled"]; self.next_save=0
        self.resize(*settings.data.get("last_window_size", [900,760])); self.image=QLabel("Connecting..."); self.image.setAlignment(Qt.AlignmentFlag.AlignCenter); self.image.setMinimumSize(640,480); self.status=QLabel("Connecting..."); self.capture_notice=QLabel(); self.capture_notice.setStyleSheet("color: #8ab89a; font-size: 11px;"); self.capture_notice.setMinimumHeight(18)
        self.toggle=QPushButton(); self.set_capture_button(self.enabled); self.toggle.clicked.connect(self.toggle_capture); connect=QPushButton("Connect"); connect.clicked.connect(self.connect); choose=QPushButton("Capture Directory"); choose.clicked.connect(self.choose_dir); quit_button=QPushButton("Quit"); quit_button.clicked.connect(self.close)
        status_layout=QVBoxLayout(); status_layout.setContentsMargins(0,0,0,0); status_layout.addWidget(self.status); status_layout.addWidget(self.capture_notice); controls=QHBoxLayout(); controls.addLayout(status_layout,1); controls.addWidget(self.toggle); controls.addWidget(connect); controls.addWidget(choose); controls.addWidget(quit_button); root=QVBoxLayout(); root.addWidget(self.image,1); root.addLayout(controls); central=QWidget(); central.setLayout(root); self.setCentralWidget(central); self.timer=QTimer(self); self.timer.timeout.connect(self.poll); self.timer.start(30); threading.Thread(target=self.stream_worker,daemon=True).start(); threading.Thread(target=self.save_worker,daemon=True).start()
    def stream_worker(self):
        stream=frames(self.settings.data["stream_url"], self.settings.data.get("username"), self.settings.data.get("password"))
        times=[]
        try:
            while not self.stop.is_set():
                frame=next(stream)
                now=time.time(); times=[t for t in times if now-t<2]; times.append(now); fps=len(times)/max(now-times[0], .001)
                if self.enabled and now>=self.next_save:
                    self.next_save=now+float(self.settings.data["capture_interval"])
                    try: self.saves.put_nowait(frame.copy())
                    except Exception: pass
                try: self.frames.get_nowait()
                except Empty: pass
                self.frames.put_nowait((frame,f"{frame.shape[1]}x{frame.shape[0]} | {fps:.1f} FPS"))
        except Exception as error: self.frames.put((None, f"Unable to connect: {error}"))
    def save_worker(self):
        while not self.stop.is_set():
            try: frame=self.saves.get(timeout=.1)
            except Empty: continue
            directory=Path(self.settings.data["capture_dir"]); directory.mkdir(parents=True,exist_ok=True); path=directory/f"capture_{datetime.now():%Y%m%d_%H%M%S_%f}.jpg"
            if cv2.imwrite(str(path),frame):
                try: self.notifications.put_nowait(path)
                except Exception: pass
    def poll(self):
        try: frame, label=self.frames.get_nowait()
        except Empty: return
        self.status.setText(label)
        try:
            latest=self.notifications.get_nowait()
            self.capture_notice.setText(f"Captured: {latest}")
            self.capture_notice.setToolTip(str(latest))
        except Empty: pass
        if frame is not None:
            rgb=cv2.cvtColor(frame,cv2.COLOR_BGR2RGB); h,w=rgb.shape[:2]; self.image.setPixmap(QPixmap.fromImage(QImage(rgb.data,w,h,rgb.strides[0],QImage.Format.Format_RGB888).copy()).scaled(self.image.size(),Qt.AspectRatioMode.KeepAspectRatio,Qt.TransformationMode.SmoothTransformation))
    def set_capture_button(self, active): self.toggle.setText("Stop Capture" if active else "Start Capture"); self.toggle.setStyleSheet(STOP_STYLE if active else START_STYLE)
    def toggle_capture(self): self.enabled=not self.enabled; self.set_capture_button(self.enabled); self.settings.data["auto_capture_enabled"]=self.enabled; self.settings.save()
    def choose_dir(self):
        path=QFileDialog.getExistingDirectory(self,"Capture directory",self.settings.data["capture_dir"])
        if path: self.settings.data["capture_dir"]=path; self.settings.save()
    def connect(self):
        dialog=ConnectDialog(self.settings.data["stream_url"], self.settings.data.get("username", "admin"), self.settings.data.get("password", ""))
        if dialog.exec() == QDialog.DialogCode.Accepted:
            self.settings.data.update(stream_url=dialog.url.text().strip(), username=dialog.username.text(), password=dialog.password.text()); self.settings.save(); QMessageBox.information(self,"Reconnect","Restart the viewer to apply the new stream settings.")
    def closeEvent(self,event): self.settings.data["last_window_size"]=[self.width(),self.height()]; self.settings.save(); self.stop.set(); event.accept()

if __name__ == "__main__":
    app=QApplication(sys.argv); settings=Settings(); dialog=ConnectDialog(settings.data["stream_url"], settings.data.get("username", "admin"), settings.data.get("password", ""))
    if dialog.exec() == QDialog.DialogCode.Accepted: settings.data.update(stream_url=dialog.url.text().strip(), username=dialog.username.text(), password=dialog.password.text()); settings.save()
    else: sys.exit(0)
    window=Viewer(settings); window.show(); sys.exit(app.exec())
