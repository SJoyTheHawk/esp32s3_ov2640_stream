#!/usr/bin/env python3
"""Stream worker thread for MJPEG capture"""
import sys
import time
from pathlib import Path

import cv2
import numpy as np
from PyQt6.QtCore import QThread, pyqtSignal

# Add parent directory to path for mjpeg_stream import
sys.path.insert(0, str(Path(__file__).parent.parent.parent))
from mjpeg_stream import frames


class StreamWorker(QThread):
    """Worker thread for reading MJPEG stream"""

    frame_ready = pyqtSignal(np.ndarray)  # Emits BGR frame
    fps_update = pyqtSignal(float)
    error_signal = pyqtSignal(str)
    connected_signal = pyqtSignal()
    reconnecting_signal = pyqtSignal(float)

    def __init__(self, url: str, username: str = None, password: str = None,
                 auto_reconnect: bool = True, reconnect_delay: float = 5):
        super().__init__()
        self.url = url
        self.username = username
        self.password = password
        self.auto_reconnect = auto_reconnect
        self.reconnect_delay = max(0, reconnect_delay)
        self.running = True
        self.frame_times = []

    def run(self):
        """Main thread loop"""
        while self.running:
            stream = None
            connected = False
            try:
                stream = frames(self.url, self.username, self.password)

                while self.running:
                    frame = next(stream)

                    if not connected:
                        connected = True
                        self.frame_times.clear()
                        self.connected_signal.emit()

                    # Calculate FPS
                    now = time.time()
                    self.frame_times = [t for t in self.frame_times if now - t < 2.0]
                    self.frame_times.append(now)

                    if len(self.frame_times) > 1:
                        fps = len(self.frame_times) / (self.frame_times[-1] - self.frame_times[0])
                        self.fps_update.emit(fps)

                    # Emit frame
                    self.frame_ready.emit(frame)

            except StopIteration:
                if self.running:
                    self.error_signal.emit("Stream ended")
            except Exception as e:
                if self.running:
                    context = "Stream interrupted" if connected else "Connection failed"
                    self.error_signal.emit(f"{context}: {e}")
            finally:
                if stream is not None:
                    stream.close()

            if not self.running or not self.auto_reconnect:
                break

            self.reconnecting_signal.emit(self.reconnect_delay)
            self._wait_before_reconnect()

    def _wait_before_reconnect(self):
        """Wait for the retry delay while allowing prompt shutdown."""
        deadline = time.monotonic() + self.reconnect_delay
        while self.running:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return
            self.msleep(min(100, max(1, int(remaining * 1000))))

    def stop(self):
        """Stop the worker thread"""
        self.running = False
