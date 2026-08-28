#!/usr/bin/env python3
"""
ESP32 Camera Professional Viewer
Advanced camera viewer with burst mode, video recording, and professional UI
"""

import sys
from pathlib import Path
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QPushButton, QLabel, QStatusBar,
                             QLineEdit, QFileDialog, QMessageBox)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont, QIcon

from widgets.stream_widget import StreamWidget
from widgets.burst_button import BurstButton
from widgets.settings_dialog import SettingsDialog
from workers.stream_worker import StreamWorker
from workers.save_worker import SaveWorker
from workers.video_worker import VideoWorker
from utils.config_manager import ConfigManager
from utils.file_manager import FileManager


class CameraViewerPro(QMainWindow):
    """Professional ESP32 Camera Viewer"""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("ESP32 Camera Viewer Pro")
        self.setMinimumSize(1000, 800)

        # Configuration
        self.config = ConfigManager()
        self.file_manager = FileManager(self.config.get('capture.save_directory'))

        # Workers
        self.stream_worker = None
        self.save_worker = SaveWorker(quality=self.config.get('capture.photo_quality'))
        self.video_worker = VideoWorker()

        # State
        self.is_recording = False
        self.burst_active = False
        self.frame_count = 0
        self.photo_count = 0
        self.video_count = 0

        # Setup UI
        self.init_ui()
        self.load_stylesheet()
        self.restore_geometry()

        # Start stream
        self.connect_stream()

    def init_ui(self):
        """Initialize user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(15, 15, 15, 15)
        main_layout.setSpacing(15)

        # Top bar - Connection
        top_bar = QHBoxLayout()
        url_label = QLabel("Stream URL:")
        url_label.setStyleSheet("font-size: 16px; font-weight: 600;")
        top_bar.addWidget(url_label)
        self.url_input = QLineEdit(self.config.get('stream.url'))
        self.url_input.setMinimumWidth(350)
        self.url_input.setMinimumHeight(40)
        self.url_input.setStyleSheet("font-size: 15px;")
        self.url_input.returnPressed.connect(self.reconnect_stream)
        top_bar.addWidget(self.url_input)
        self.connect_btn = QPushButton("Reconnect")
        self.connect_btn.setMinimumHeight(40)
        self.connect_btn.setStyleSheet("font-size: 15px;")
        self.connect_btn.clicked.connect(self.reconnect_stream)
        top_bar.addWidget(self.connect_btn)
        top_bar.addStretch()
        main_layout.addLayout(top_bar)

        # Stream preview
        self.stream_widget = StreamWidget()
        self.stream_widget.setMinimumSize(800, 600)
        main_layout.addWidget(self.stream_widget, stretch=1)

        # Control panel
        controls = self.create_control_panel()
        main_layout.addLayout(controls)

        # Status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)

        self.connection_label = QLabel("● Disconnected")
        self.connection_label.setStyleSheet("color: #ff4444; font-weight: bold; font-size: 15px;")
        self.status_bar.addPermanentWidget(self.connection_label)

        self.stats_label = QLabel("Captured: 0 photos | 0 videos")
        self.stats_label.setStyleSheet("font-size: 15px;")
        self.status_bar.addPermanentWidget(self.stats_label)

    def create_control_panel(self):
        """Create capture control panel"""
        controls = QHBoxLayout()
        controls.setSpacing(10)

        # Unified capture button (single press or long press for burst)
        self.capture_btn = BurstButton("📷 Capture")
        self.capture_btn.setMinimumSize(140, 70)
        self.capture_btn.setStyleSheet("font-size: 17px; font-weight: 600;")
        self.capture_btn.setToolTip("Click to capture single frame, hold for burst")
        self.capture_btn.short_press_signal.connect(self.capture_single)
        self.capture_btn.pressed_signal.connect(self.start_burst)
        self.capture_btn.released_signal.connect(self.stop_burst)
        controls.addWidget(self.capture_btn)

        # Video recording button
        self.record_btn = QPushButton("🎥 Record")
        self.record_btn.setMinimumSize(140, 70)
        self.record_btn.setStyleSheet("font-size: 17px; font-weight: 600;")
        self.record_btn.setCheckable(True)
        self.record_btn.setToolTip("Start/stop video recording")
        self.record_btn.clicked.connect(self.toggle_recording)
        controls.addWidget(self.record_btn)

        # Settings button
        self.settings_btn = QPushButton("⚙ Settings")
        self.settings_btn.setMinimumSize(140, 70)
        self.settings_btn.setStyleSheet("font-size: 17px; font-weight: 600;")
        self.settings_btn.setToolTip("Open settings dialog")
        self.settings_btn.clicked.connect(self.show_settings)
        controls.addWidget(self.settings_btn)

        controls.addStretch()
        return controls

    def connect_stream(self):
        """Connect to MJPEG stream"""
        url = self.config.get('stream.url')
        username = self.config.get('stream.username')
        password = self.config.get('stream.password')
        auto_reconnect = self.config.get('stream.auto_reconnect', True)
        reconnect_delay = self.config.get('stream.reconnect_delay', 5)

        self.stream_worker = StreamWorker(
            url,
            username,
            password,
            auto_reconnect=auto_reconnect,
            reconnect_delay=reconnect_delay
        )
        self.stream_worker.frame_ready.connect(self.stream_widget.update_frame)
        self.stream_worker.fps_update.connect(self.stream_widget.set_fps)
        self.stream_worker.error_signal.connect(self.handle_stream_error)
        self.stream_worker.connected_signal.connect(self.on_stream_connected)
        self.stream_worker.reconnecting_signal.connect(self.on_stream_reconnecting)
        self.stream_worker.start()

        self.status_bar.showMessage("Connecting to stream...", 3000)

    def reconnect_stream(self):
        """Reconnect to stream with new URL"""
        if self.stream_worker:
            self.stream_worker.stop()
            self.stream_worker.wait(2000)

        # Update URL in config
        self.config.set('stream.url', self.url_input.text())
        self.connect_stream()

    def on_stream_connected(self):
        """Handle stream connection success"""
        self.connection_label.setText("● Connected")
        self.connection_label.setStyleSheet("color: #44ff44; font-weight: bold; font-size: 15px;")
        self.status_bar.showMessage("Stream connected", 3000)

    def handle_stream_error(self, error):
        """Handle stream errors"""
        self.connection_label.setText("● Error")
        self.connection_label.setStyleSheet("color: #ff4444; font-weight: bold; font-size: 15px;")
        self.status_bar.showMessage(f"Stream error: {error}", 5000)

    def on_stream_reconnecting(self, delay):
        """Show that the stream worker is waiting for another attempt."""
        self.connection_label.setText("● Reconnecting")
        self.connection_label.setStyleSheet(
            "color: #ffaa00; font-weight: bold; font-size: 15px;"
        )
        self.status_bar.showMessage(f"Reconnecting in {delay:g} seconds...", 0)

    def capture_single(self):
        """Capture single frame"""
        frame = self.stream_widget.get_current_frame()
        if frame is not None:
            filename = self.file_manager.generate_photo_filename()
            self.save_worker.save_frame(frame, filename)
            self.photo_count += 1
            self.status_bar.showMessage(f"Saved: {filename.name}", 3000)
            self.update_stats()
        else:
            self.status_bar.showMessage("No frame available", 2000)

    def start_burst(self):
        """Start burst capture mode"""
        self.burst_active = True
        self.frame_count = 0
        self.file_manager.reset_burst_session()

        fps = self.config.get('capture.burst_fps', 5)
        interval = int(1000 / fps)  # milliseconds

        self.burst_timer = QTimer()
        self.burst_timer.timeout.connect(self.capture_burst_frame)
        self.burst_timer.start(interval)

        self.status_bar.showMessage("Burst mode active - hold button", 0)

    def capture_burst_frame(self):
        """Capture one frame during burst"""
        frame = self.stream_widget.get_current_frame()
        if frame is not None:
            self.frame_count += 1
            filename = self.file_manager.generate_burst_filename(self.frame_count)
            self.save_worker.save_frame(frame, filename)
            self.capture_btn.set_frame_count(self.frame_count)

    def stop_burst(self):
        """Stop burst capture mode"""
        if hasattr(self, 'burst_timer'):
            self.burst_timer.stop()

        self.burst_active = False
        self.photo_count += self.frame_count
        self.status_bar.showMessage(f"Burst complete: {self.frame_count} frames", 3000)
        self.capture_btn.reset()
        self.update_stats()

    def toggle_recording(self, checked):
        """Toggle video recording"""
        if checked:
            # Prompt for filename
            default_name = str(self.file_manager.generate_video_filename())
            filename, _ = QFileDialog.getSaveFileName(
                self,
                "Save Video",
                default_name,
                "Video Files (*.avi *.mp4);;All Files (*)"
            )

            if filename:
                self.start_recording(Path(filename))
            else:
                self.record_btn.setChecked(False)
        else:
            self.stop_recording()

    def start_recording(self, filename: Path):
        """Start video recording"""
        fps = self.config.get('video.fps', 20)
        codec = self.config.get('video.codec', 'MJPEG')

        self.video_worker.start_recording(filename, fps, codec)
        self.stream_widget.frame_updated.connect(self.video_worker.add_frame)

        self.is_recording = True
        self.record_btn.setText("⏹ Stop")
        self.status_bar.showMessage(f"Recording to: {filename.name}", 0)

    def stop_recording(self):
        """Stop video recording"""
        self.stream_widget.frame_updated.disconnect(self.video_worker.add_frame)
        self.video_worker.stop_recording()

        self.is_recording = False
        self.video_count += 1
        self.record_btn.setText("🎥 Record")
        self.record_btn.setChecked(False)
        self.status_bar.showMessage("Recording stopped", 3000)
        self.update_stats()

    def show_settings(self):
        """Show settings dialog"""
        dialog = SettingsDialog(self.config, self)
        if dialog.exec():
            # Settings saved, reload if needed
            self.file_manager.save_directory = Path(self.config.get('capture.save_directory'))
            self.save_worker.quality = self.config.get('capture.photo_quality')
            QMessageBox.information(
                self,
                "Settings Saved",
                "Settings have been saved successfully."
            )

    def update_stats(self):
        """Update statistics display"""
        self.stats_label.setText(
            f"Captured: {self.photo_count} photos | {self.video_count} videos"
        )

    def load_stylesheet(self):
        """Load dark theme stylesheet"""
        theme = self.config.get('ui.theme', 'dark')
        if theme == 'dark':
            style_file = Path(__file__).parent / 'styles' / 'dark_theme.qss'
            if style_file.exists():
                with open(style_file, 'r') as f:
                    self.setStyleSheet(f.read())

    def restore_geometry(self):
        """Restore window geometry from config"""
        geometry = self.config.get('ui.window_geometry')
        if geometry and len(geometry) == 4:
            self.setGeometry(*geometry)

    def closeEvent(self, event):
        """Handle application close"""
        # Save window geometry
        geo = self.geometry()
        self.config.set('ui.window_geometry', [geo.x(), geo.y(), geo.width(), geo.height()])

        # Stop recording if active
        if self.is_recording:
            self.stop_recording()

        # Stop workers
        if self.stream_worker:
            self.stream_worker.stop()
            self.stream_worker.wait(2000)

        self.save_worker.stop()

        event.accept()


def main():
    """Main entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("ESP32 Camera Viewer Pro")

    # Set default font - larger for better readability
    font = QFont("Segoe UI", 14)
    app.setFont(font)

    window = CameraViewerPro()
    window.show()

    sys.exit(app.exec())


if __name__ == '__main__':
    main()
