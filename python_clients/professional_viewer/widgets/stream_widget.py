#!/usr/bin/env python3
"""Stream display widget"""
import cv2
import numpy as np
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QImage, QPixmap, QPainter, QFont, QColor
from PyQt6.QtWidgets import QLabel


class StreamWidget(QLabel):
    """Widget for displaying live stream with FPS overlay"""

    frame_updated = pyqtSignal(np.ndarray)  # Emits when frame changes

    def __init__(self):
        super().__init__()
        self.current_frame = None
        self.fps = 0.0
        self.resolution = ""

        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setStyleSheet("background-color: #000000; border: 1px solid #334155;")
        self.setMinimumSize(640, 480)
        self.setText("Connecting...")
        self.setFont(QFont("Arial", 18))

    def update_frame(self, frame: np.ndarray):
        """Update displayed frame"""
        self.current_frame = frame.copy()
        h, w = frame.shape[:2]
        self.resolution = f"{w}x{h}"

        # Convert BGR to RGB
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Create QImage
        bytes_per_line = rgb.strides[0]
        qimg = QImage(rgb.data, w, h, bytes_per_line, QImage.Format.Format_RGB888)

        # Scale to fit widget
        pixmap = QPixmap.fromImage(qimg).scaled(
            self.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )

        self.setPixmap(pixmap)
        self.frame_updated.emit(frame)

    def set_fps(self, fps: float):
        """Update FPS display"""
        self.fps = fps
        self.update()

    def get_current_frame(self) -> np.ndarray:
        """Get current frame (copy)"""
        if self.current_frame is not None:
            return self.current_frame.copy()
        return None

    def paintEvent(self, event):
        """Custom paint to add FPS overlay"""
        super().paintEvent(event)

        if self.fps > 0:
            painter = QPainter(self)
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)

            # Draw FPS badge in top-left corner
            text = f"{self.fps:.1f} FPS | {self.resolution}"
            font = QFont("Arial", 14, QFont.Weight.Bold)
            painter.setFont(font)

            # Background
            painter.fillRect(5, 5, 220, 32, QColor(0, 0, 0, 180))

            # Text
            painter.setPen(QColor(56, 189, 248))  # Cyan
            painter.drawText(10, 26, text)
