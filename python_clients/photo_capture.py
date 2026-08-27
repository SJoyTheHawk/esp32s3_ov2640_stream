#!/usr/bin/env python3
"""Save still frames from an ESP32 MJPEG stream."""
import argparse
from datetime import datetime
from pathlib import Path
import cv2
from mjpeg_stream import frames, credentials

def capture(url: str, directory: Path, username=None, password=None) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    print("Press s to save a photo, q to quit.")
    try:
        for frame in frames(url, username, password):
            cv2.imshow("ESP32 Camera", frame)
            key = cv2.waitKey(1) & 0xFF
            if key == ord("q"): break
            if key == ord("s"):
                path = directory / f"photo_{datetime.now():%Y%m%d_%H%M%S_%f}.jpg"
                cv2.imwrite(str(path), frame)
                print(f"Saved {path}")
    finally:
        cv2.destroyAllWindows()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="http://192.168.2.100/stream")
    parser.add_argument("--output", type=Path, default=Path("captures"))
    credentials(parser); args = parser.parse_args(); capture(args.url, args.output, args.username, args.password)
