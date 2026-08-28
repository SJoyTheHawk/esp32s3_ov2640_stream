#!/usr/bin/env python3
"""Save worker for background file saving"""
import cv2
import numpy as np
from pathlib import Path
from queue import Queue, Empty
from threading import Thread


class SaveWorker:
    """Background worker for saving frames to disk"""

    def __init__(self, quality: int = 95):
        self.quality = quality
        self.queue = Queue(maxsize=10)
        self.running = True
        self.thread = Thread(target=self._worker, daemon=True)
        self.thread.start()

    def _worker(self):
        """Worker thread that processes save queue"""
        while self.running:
            try:
                frame, filepath = self.queue.get(timeout=0.5)

                # Ensure parent directory exists
                filepath.parent.mkdir(parents=True, exist_ok=True)

                # Save with specified quality
                cv2.imwrite(
                    str(filepath),
                    frame,
                    [cv2.IMWRITE_JPEG_QUALITY, self.quality]
                )

            except Empty:
                continue
            except Exception as e:
                print(f"Save error: {e}")

    def save_frame(self, frame: np.ndarray, filepath: Path):
        """Queue a frame for saving"""
        try:
            self.queue.put_nowait((frame.copy(), filepath))
        except Exception as e:
            print(f"Queue full, dropping frame: {e}")

    def stop(self):
        """Stop the worker thread"""
        self.running = False
        if self.thread.is_alive():
            self.thread.join(timeout=2.0)
