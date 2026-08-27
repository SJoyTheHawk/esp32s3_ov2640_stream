#!/usr/bin/env python3
"""Display an ESP32 MJPEG stream with OpenCV."""
import argparse
import cv2
from mjpeg_stream import frames, credentials

def view_stream(url: str, username=None, password=None) -> None:
    print("Connected. Press q to quit.")
    try:
        for frame in frames(url, username, password):
            cv2.imshow("ESP32 Camera", frame)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break
    finally:
        cv2.destroyAllWindows()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default="http://192.168.2.100/stream")
    credentials(parser); args=parser.parse_args(); view_stream(args.url, args.username, args.password)
