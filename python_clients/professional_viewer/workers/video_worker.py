#!/usr/bin/env python3
"""Video worker for recording to file"""
import cv2
import numpy as np
from pathlib import Path
from threading import Lock


class VideoWorker:
    """Worker for video recording"""

    def __init__(self):
        self.writer = None
        self.filepath = None
        self.fps = 20
        self.codec = 'MJPEG'
        self.lock = Lock()
        self.is_recording = False

    def start_recording(self, filepath: Path, fps: int = 20, codec: str = 'MJPEG'):
        """Start recording video"""
        with self.lock:
            if self.is_recording:
                self.stop_recording()

            self.filepath = filepath
            self.fps = fps
            self.codec = codec

            # Ensure parent directory exists
            filepath.parent.mkdir(parents=True, exist_ok=True)

            # Create VideoWriter (will be initialized on first frame)
            self.writer = None
            self.is_recording = True

            print(f"Recording started: {filepath}")

    def add_frame(self, frame: np.ndarray):
        """Add frame to video"""
        with self.lock:
            if not self.is_recording:
                return

            # Initialize writer on first frame
            if self.writer is None:
                h, w = frame.shape[:2]
                fourcc = cv2.VideoWriter_fourcc(*self.codec)
                self.writer = cv2.VideoWriter(
                    str(self.filepath),
                    fourcc,
                    self.fps,
                    (w, h)
                )

                if not self.writer.isOpened():
                    print("Failed to open video writer")
                    self.is_recording = False
                    return

            # Write frame
            self.writer.write(frame)

    def stop_recording(self):
        """Stop recording and close file"""
        with self.lock:
            if self.writer is not None:
                self.writer.release()
                self.writer = None
                print(f"Recording stopped: {self.filepath}")

            self.is_recording = False
            self.filepath = None
