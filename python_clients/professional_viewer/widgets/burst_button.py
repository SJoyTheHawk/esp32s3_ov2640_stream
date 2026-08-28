#!/usr/bin/env python3
"""Custom burst button with long-press detection and charging animation"""
from PyQt6.QtWidgets import QPushButton
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QRectF
from PyQt6.QtGui import QPainter, QColor, QPen, QFont


class BurstButton(QPushButton):
    """Custom button with short press (single capture) and long-press (burst) detection"""

    short_press_signal = pyqtSignal()  # Single click capture
    pressed_signal = pyqtSignal()  # Long press burst start
    released_signal = pyqtSignal()
    charging_signal = pyqtSignal(float)  # Progress 0.0-1.0

    def __init__(self, text="📷 Capture", parent=None):
        super().__init__(text, parent)

        self.press_duration = 0
        self.charge_threshold = 2000  # 2 seconds to activate burst
        self.frame_count = 0
        self.is_charging = False
        self.is_burst_active = False  # Track if burst mode is running
        self.original_text = text

        # Timer for press duration tracking
        self.press_timer = QTimer()
        self.press_timer.timeout.connect(self._update_charging)
        self.press_timer.setInterval(50)  # Update every 50ms

    def mousePressEvent(self, event):
        """Handle mouse press"""
        super().mousePressEvent(event)
        if event.button() == Qt.MouseButton.LeftButton:
            # If burst is already active, this click stops it
            if self.is_burst_active:
                self.released_signal.emit()
                self.is_burst_active = False
                return

            # Otherwise, start charging for burst
            self.press_duration = 0
            self.is_charging = True
            self.press_timer.start()

    def mouseReleaseEvent(self, event):
        """Handle mouse release"""
        super().mouseReleaseEvent(event)
        if event.button() == Qt.MouseButton.LeftButton:
            # If burst hasn't activated yet
            if not self.is_burst_active:
                self.press_timer.stop()

                # If released before threshold, it's a short press - single capture
                if self.press_duration < self.charge_threshold:
                    self.short_press_signal.emit()
                # If threshold reached, burst is now active (continues running)
                # User must click again to stop

                self.is_charging = False
                self.press_duration = 0
                self.update()

    def _update_charging(self):
        """Update charging animation"""
        self.press_duration += 50

        # Emit burst start after threshold (2 seconds)
        if self.press_duration == self.charge_threshold:
            self.is_burst_active = True
            self.pressed_signal.emit()

        # Calculate progress (0.0 to 1.0)
        progress = min(1.0, self.press_duration / self.charge_threshold)
        self.charging_signal.emit(progress)

        self.update()  # Trigger repaint

    def paintEvent(self, event):
        """Custom paint with charging ring"""
        super().paintEvent(event)

        if not self.is_charging and not self.is_burst_active and self.frame_count == 0:
            return

        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        # Calculate ring position and size
        rect = self.rect()
        center_x = rect.center().x()
        center_y = rect.center().y()
        radius = min(rect.width(), rect.height()) // 2 - 10

        # Draw charging ring or burst active indicator
        if self.is_charging or self.is_burst_active:
            progress = min(1.0, self.press_duration / self.charge_threshold)

            # Color transition: blue → green → yellow → red (fully charged)
            if progress < 0.33:
                color = QColor(66, 135, 245)  # Blue
            elif progress < 0.66:
                color = QColor(76, 209, 55)  # Green
            elif progress < 1.0:
                color = QColor(245, 166, 35)  # Yellow
            else:
                color = QColor(220, 38, 38)  # Red (burst active)

            # Draw arc
            pen = QPen(color, 4)
            painter.setPen(pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)

            start_angle = 90 * 16  # Start from top
            span_angle = int(-progress * 360 * 16)  # Clockwise

            arc_rect = QRectF(
                center_x - radius,
                center_y - radius,
                radius * 2,
                radius * 2
            )
            painter.drawArc(arc_rect, start_angle, span_angle)

        # Draw frame count
        if self.frame_count > 0:
            font = QFont("Arial", 16, QFont.Weight.Bold)
            painter.setFont(font)
            painter.setPen(QColor(255, 255, 255))
            painter.drawText(rect, Qt.AlignmentFlag.AlignCenter, str(self.frame_count))

    def set_frame_count(self, count: int):
        """Set frame count display"""
        self.frame_count = count
        self.update()

    def reset(self):
        """Reset button state"""
        self.frame_count = 0
        self.press_duration = 0
        self.is_charging = False
        self.is_burst_active = False
        self.setText(self.original_text)
        self.update()
