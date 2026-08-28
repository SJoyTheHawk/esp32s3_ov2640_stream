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

    def __init__(self, url: str, username: str = None, password: str = None):
        super().__init__()
        self.url = url
        self.username = username
        self.password = password
        self.running = True
        self.frame_times = []

    def run(self):
        """Main thread loop"""
        try:
            # Connect to stream using mjpeg_stream helper
            stream = frames(self.url, self.username, self.password)
            self.connected_signal.emit()

            while self.running:
                try:
                    frame = next(stream)

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
                    self.error_signal.emit("Stream ended")
                    break
                except Exception as e:
                    self.error_signal.emit(f"Frame read error: {e}")
                    time.sleep(0.1)

        except Exception as e:
            self.error_signal.emit(f"Connection failed: {e}")

    def stop(self):
        """Stop the worker thread"""
        self.running = False
