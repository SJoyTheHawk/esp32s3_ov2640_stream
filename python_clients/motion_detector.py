#!/usr/bin/env python3
"""Save frames when motion is detected in an ESP32 stream."""
import argparse
from datetime import datetime
from pathlib import Path
import cv2
from mjpeg_stream import frames, credentials

def detect(url: str, directory: Path, threshold: float, username=None, password=None) -> None:
    directory.mkdir(parents=True, exist_ok=True); previous = None
    try:
        for frame in frames(url, username, password):
            gray = cv2.GaussianBlur(cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY), (21, 21), 0)
            if previous is not None:
                delta = cv2.absdiff(previous, gray); _, mask = cv2.threshold(delta, 25, 255, cv2.THRESH_BINARY)
                ratio = cv2.countNonZero(mask) / mask.size
                if ratio >= threshold:
                    path = directory / f"motion_{datetime.now():%Y%m%d_%H%M%S_%f}.jpg"
                    cv2.imwrite(str(path), frame); print(f"Motion {ratio:.3f}: {path}")
            previous = gray
            cv2.imshow("Motion Detector", frame)
            if cv2.waitKey(1) & 0xFF == ord("q"): break
    finally:
        cv2.destroyAllWindows()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("--url", default="http://192.168.2.100/stream")
    parser.add_argument("--output", type=Path, default=Path("motion_captures")); parser.add_argument("--threshold", type=float, default=0.02)
    credentials(parser); args = parser.parse_args(); detect(args.url, args.output, args.threshold, args.username, args.password)
