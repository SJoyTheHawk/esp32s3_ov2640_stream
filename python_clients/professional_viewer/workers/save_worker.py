#!/usr/bin/env python3
"""Save worker for background file saving"""
import cv2
import json
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
                frame, filepath, metadata = self.queue.get(timeout=0.5)

                # Ensure parent directory exists
                filepath.parent.mkdir(parents=True, exist_ok=True)

                # Save with specified quality
                saved = cv2.imwrite(
                    str(filepath),
                    frame,
                    [cv2.IMWRITE_JPEG_QUALITY, self.quality]
                )
                if saved and metadata is not None:
                    filepath.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')

            except Empty:
                continue
            except Exception as e:
                print(f"Save error: {e}")

    def save_frame(self, frame: np.ndarray, filepath: Path, metadata=None):
        """Queue a frame for saving"""
        try:
            self.queue.put_nowait((frame.copy(), filepath, metadata))
        except Exception as e:
            print(f"Queue full, dropping frame: {e}")

    def stop(self):
        """Stop the worker thread"""
        self.running = False
        if self.thread.is_alive():
            self.thread.join(timeout=2.0)
