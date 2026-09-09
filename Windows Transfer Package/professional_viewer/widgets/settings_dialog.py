#!/usr/bin/env python3
"""Settings dialog for configuration"""
from PyQt6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                             QLineEdit, QSpinBox, QComboBox, QPushButton,
                             QFileDialog, QGroupBox, QFormLayout)
from PyQt6.QtCore import Qt


class SettingsDialog(QDialog):
    """Settings configuration dialog"""

    def __init__(self, config_manager, parent=None):
        super().__init__(parent)
        self.config = config_manager
        self.setWindowTitle("Settings")
        self.setMinimumWidth(500)
        self.setup_ui()

    def setup_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout(self)

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

        # Buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()

        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(cancel_btn)

        save_btn = QPushButton("Save")
        save_btn.clicked.connect(self.save_settings)
        save_btn.setDefault(True)
        button_layout.addWidget(save_btn)

        layout.addLayout(button_layout)

    def browse_directory(self):
        """Open directory browser"""
        directory = QFileDialog.getExistingDirectory(
            self,
            "Select Save Directory",
            self.save_dir_input.text()
        )
        if directory:
            self.save_dir_input.setText(directory)

    def save_settings(self):
        """Save settings and close"""
        self.config.set('stream.url', self.url_input.text())
        self.config.set('stream.username', self.username_input.text())
        self.config.set('stream.password', self.password_input.text())
        self.config.set('capture.save_directory', self.save_dir_input.text())
        self.config.set('capture.burst_fps', self.burst_fps_spin.value())
        self.config.set('capture.photo_quality', self.quality_spin.value())
        self.config.set('video.fps', self.video_fps_spin.value())
        self.config.set('video.codec', self.codec_combo.currentText())

        self.accept()
