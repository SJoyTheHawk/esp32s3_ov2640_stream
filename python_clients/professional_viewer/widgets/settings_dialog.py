#!/usr/bin/env python3
"""Settings dialog for configuration"""
from PyQt6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                             QLineEdit, QSpinBox, QComboBox, QPushButton,
                             QFileDialog, QGroupBox, QFormLayout, QCheckBox, QSlider,
                             QScrollArea, QWidget)
from PyQt6.QtCore import Qt
from workers.led_worker import LedWorker


class SettingsDialog(QDialog):
    """Settings configuration dialog"""

    def __init__(self, config_manager, parent=None):
        super().__init__(parent)
        self.config = config_manager
        self.setWindowTitle("Settings")
        self.setMinimumWidth(500)
        self.led_worker = None
        self.led_state = None
        self.led_connection = None
        self.setup_ui()
        self.load_led()

    def setup_ui(self):
        """Initialize UI"""
        root = QVBoxLayout(self)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        self.form = QWidget()
        layout = QVBoxLayout(self.form)
        scroll.setWidget(self.form)
        root.addWidget(scroll)
        self.resize(560, 740)

        # Stream settings
        stream_group = QGroupBox("Stream")
        stream_layout = QFormLayout()

        self.url_input = QLineEdit(self.config.get('stream.url'))
        stream_layout.addRow("URL:", self.url_input)

        self.username_input = QLineEdit(self.config.get('stream.username', ''))
        stream_layout.addRow("Username:", self.username_input)

        self.password_input = QLineEdit(self.config.get('stream.password', ''))
        self.password_input.setEchoMode(QLineEdit.EchoMode.Password)
        stream_layout.addRow("Password:", self.password_input)

        stream_group.setLayout(stream_layout)
        layout.addWidget(stream_group)

        # Capture settings
        capture_group = QGroupBox("Capture")
        capture_layout = QFormLayout()

        dir_layout = QHBoxLayout()
        self.save_dir_input = QLineEdit(self.config.get('capture.save_directory'))
        dir_layout.addWidget(self.save_dir_input)
        browse_btn = QPushButton("Browse")
        browse_btn.clicked.connect(self.browse_directory)
        dir_layout.addWidget(browse_btn)
        capture_layout.addRow("Save Directory:", dir_layout)

        self.burst_fps_spin = QSpinBox()
        self.burst_fps_spin.setRange(1, 30)
        self.burst_fps_spin.setValue(self.config.get('capture.burst_fps'))
        self.burst_fps_spin.setMinimumHeight(40)
        self.burst_fps_spin.setStyleSheet("font-size: 15px;")
        capture_layout.addRow("Burst FPS:", self.burst_fps_spin)

        self.quality_spin = QSpinBox()
        self.quality_spin.setRange(10, 100)
        self.quality_spin.setValue(self.config.get('capture.photo_quality'))
        self.quality_spin.setMinimumHeight(40)
        self.quality_spin.setStyleSheet("font-size: 15px;")
        capture_layout.addRow("JPEG Quality:", self.quality_spin)

        capture_group.setLayout(capture_layout)
        layout.addWidget(capture_group)

        # Video settings
        video_group = QGroupBox("Video")
        video_layout = QFormLayout()

        self.video_fps_spin = QSpinBox()
        self.video_fps_spin.setRange(5, 60)
        self.video_fps_spin.setValue(self.config.get('video.fps'))
        self.video_fps_spin.setMinimumHeight(40)
        self.video_fps_spin.setStyleSheet("font-size: 15px;")
        video_layout.addRow("Recording FPS:", self.video_fps_spin)

        self.codec_combo = QComboBox()
        self.codec_combo.addItems(['MJPEG', 'H264', 'XVID'])
        current_codec = self.config.get('video.codec')
        self.codec_combo.setCurrentText(current_codec)
        video_layout.addRow("Codec:", self.codec_combo)

        video_group.setLayout(video_layout)
        layout.addWidget(video_group)

        self.led_group = QGroupBox("LED light")
        led_layout = QFormLayout(self.led_group)
        self.led_enabled = QCheckBox("Enable LED light")
        led_layout.addRow(self.led_enabled)
        self.led_sliders = {}
        for key, maximum in (("red", 255), ("green", 255), ("blue", 255), ("brightness", 100)):
            row = QHBoxLayout()
            slider = QSlider(Qt.Orientation.Horizontal)
            slider.setRange(0, maximum)
            output = QLabel("0")
            output.setMinimumWidth(35)
            slider.valueChanged.connect(lambda value, label=output: label.setText(str(value)))
            row.addWidget(slider)
            row.addWidget(output)
            led_layout.addRow(key.title() + (" (%):" if key == "brightness" else ":"), row)
            self.led_sliders[key] = slider
        layout.addWidget(self.led_group)
        self.led_group.setEnabled(False)
        self.led_message = QLabel("Loading LED settings...")
        self.led_message.setWordWrap(True)
        layout.addWidget(self.led_message)
        self.led_reload = QPushButton("Reload LED settings")
        self.led_reload.clicked.connect(self.load_led)
        layout.addWidget(self.led_reload)
        for field in (self.url_input, self.username_input, self.password_input):
            field.textChanged.connect(self.connection_edited)

        # Buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()

        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(cancel_btn)

        self.save_btn = QPushButton("Save")
        self.save_btn.clicked.connect(self.save_settings)
        self.save_btn.setDefault(True)
        button_layout.addWidget(self.save_btn)

        root.addLayout(button_layout)

    def browse_directory(self):
        """Open directory browser"""
        directory = QFileDialog.getExistingDirectory(
            self,
            "Select Save Directory",
            self.save_dir_input.text()
        )
        if directory:
            self.save_dir_input.setText(directory)

    def connection(self):
        return (self.url_input.text().strip(), self.username_input.text(), self.password_input.text())

    def connection_edited(self):
        self.led_state = None
        self.led_group.setEnabled(False)
        self.led_message.setText("Connection changed. Reload LED settings before editing the light.")

    def load_led(self):
        if self.led_worker is not None:
            return
        self.led_state = None
        self.led_group.setEnabled(False)
        self.start_led_request()

    def start_led_request(self, values=None):
        self.led_message.setText("Saving LED settings..." if values is not None else "Loading LED settings...")
        self.form.setEnabled(False)
        self.save_btn.setEnabled(False)
        worker = LedWorker(self.connection(), values, self)
        self.led_worker = worker
        worker.finished.connect(self.led_finished)
        worker.start()

    def led_finished(self):
        worker = self.led_worker
        if worker is None:
            return
        self.led_worker = None
        self.form.setEnabled(True)
        self.save_btn.setEnabled(True)
        if worker.error:
            # Preserve the draft after a failed save. Loading failures leave
            # LED controls disabled while local settings remain usable.
            self.led_message.setText(worker.error)
        elif worker.values is not None:
            self.save_local_settings()
        else:
            self.led_state = worker.result
            self.led_connection = worker.connection
            self.led_enabled.setChecked(worker.result['enabled'])
            for key, slider in self.led_sliders.items():
                slider.setValue(worker.result[key])
            self.led_group.setEnabled(True)
            effective = worker.result.get('effective_brightness', 0)
            budget = worker.result.get('power_budget_ma', 300)
            self.led_message.setText(f"Applied brightness: {effective}%. Power budget: {budget} mA. "
                                     "Changes apply when you Save. Hardware settings are in the web admin page.")
        worker.deleteLater()

    def reject(self):
        if self.led_worker is not None:
            worker = self.led_worker
            worker.requestInterruption()
            if not worker.wait(7000):
                self.led_message.setText("The device request is still running; please wait.")
                return
            self.led_worker = None
            worker.deleteLater()
        super().reject()

    def closeEvent(self, event):
        if self.led_worker is not None:
            worker = self.led_worker
            worker.requestInterruption()
            if not worker.wait(7000):
                event.ignore()
                return
            self.led_worker = None
            worker.deleteLater()
        super().closeEvent(event)

    def save_settings(self):
        """Commit LED edits once, then save local preferences on success."""
        if self.led_worker is not None:
            return
        if self.led_state is not None and self.connection() == self.led_connection:
            values = {key: slider.value() for key, slider in self.led_sliders.items()}
            values['enabled'] = self.led_enabled.isChecked()
            if any(values[key] != self.led_state[key] for key in values):
                self.start_led_request(values)
                return
        self.save_local_settings()

    def save_local_settings(self):
        """LED state lives on the device, never in this JSON configuration."""
        self.config.set('stream.url', self.url_input.text())
        self.config.set('stream.username', self.username_input.text())
        self.config.set('stream.password', self.password_input.text())
        self.config.set('capture.save_directory', self.save_dir_input.text())
        self.config.set('capture.burst_fps', self.burst_fps_spin.value())
        self.config.set('capture.photo_quality', self.quality_spin.value())
        self.config.set('video.fps', self.video_fps_spin.value())
        self.config.set('video.codec', self.codec_combo.currentText())

        self.accept()
