#!/usr/bin/env python3
"""File manager for smart filename generation"""
from datetime import datetime
from pathlib import Path


class FileManager:
    """Manages file naming and directory structure for captures"""

    def __init__(self, save_directory: str = "./captures"):
        self.save_directory = Path(save_directory)
        self.save_directory.mkdir(parents=True, exist_ok=True)
        self.burst_session_timestamp = None

    def generate_photo_filename(self, prefix: str = "capture") -> Path:
        """Generate timestamped filename for single photo"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S_%f')[:-3]
        filename = f"{prefix}_{timestamp}.jpg"
        return self.save_directory / filename

    def generate_burst_filename(self, frame_number: int) -> Path:
        """Generate filename for burst capture frame"""
        if self.burst_session_timestamp is None:
            self.burst_session_timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')

        filename = f"burst_{self.burst_session_timestamp}_{frame_number:04d}.jpg"
        return self.save_directory / filename

    def reset_burst_session(self):
        """Reset burst session for new burst sequence"""
        self.burst_session_timestamp = None

    def generate_video_filename(self, name: str = None) -> Path:
        """Generate filename for video recording"""
        if name is None:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            name = f"recording_{timestamp}"

        # Ensure .avi extension
        if not name.endswith('.avi'):
            name = f"{name}.avi"

        return self.save_directory / name

    def get_unique_filename(self, filepath: Path) -> Path:
        """Get unique filename if file already exists"""
        if not filepath.exists():
            return filepath

        base = filepath.stem
        ext = filepath.suffix
        parent = filepath.parent
        counter = 1

        while True:
            new_path = parent / f"{base}_{counter}{ext}"
            if not new_path.exists():
                return new_path
            counter += 1
