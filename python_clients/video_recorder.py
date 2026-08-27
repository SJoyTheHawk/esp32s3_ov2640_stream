#!/usr/bin/env python3
"""Record an ESP32 MJPEG stream to an AVI file."""
import argparse
from pathlib import Path
import cv2
from mjpeg_stream import frames, credentials

def record(url: str, output: Path, fps: float, seconds: float | None, username=None, password=None) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    writer = None
    count = 0
    stream = frames(url, username, password)
    try:
        while seconds is None or count < seconds * fps:
            frame = next(stream)
            if writer is None:
                height, width = frame.shape[:2]
                writer = cv2.VideoWriter(str(output), cv2.VideoWriter_fourcc(*"MJPG"), fps, (width, height))
            writer.write(frame)
            count += 1
    finally:
        if writer: writer.release()
    print(f"Saved {count} frames to {output}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="http://192.168.2.100/stream")
    parser.add_argument("--output", type=Path, default=Path("recordings/camera.avi"))
    parser.add_argument("--fps", type=float, default=20)
    parser.add_argument("--seconds", type=float)
    credentials(parser); args = parser.parse_args()
    record(args.url, args.output, args.fps, args.seconds,args.username,args.password)
