"""Authenticated multipart MJPEG reader for the professional viewer."""

from typing import Iterator
from dataclasses import dataclass

import cv2
import numpy as np
import requests

@dataclass(frozen=True)
class StreamFrame:
    image: np.ndarray
    jpeg: bytes
    weight_grams: float | None = None
    weight_raw: int | None = None
    weight_age_ms: int | None = None
    weight_valid: bool = False
    weight_stable: bool = False

def frames_with_metadata(url: str, username: str | None = None, password: str | None = None,
                         timeout: float = 10) -> Iterator[StreamFrame]:
    session = requests.Session()
    if username is not None and password is not None:
        response = session.post(url.rsplit("/", 1)[0] + "/api/login",
                                data={"username": username, "password": password}, timeout=timeout)
        response.raise_for_status()
        if response.json().get("status") != "success":
            raise RuntimeError("ESP32 login failed")
    with session.get(url, stream=True, timeout=(timeout, timeout)) as response:
        response.raise_for_status(); buffer = b""
        for chunk in response.iter_content(chunk_size=8192):
            buffer += chunk
            while True:
                start = buffer.find(b"\xff\xd8"); end = buffer.find(b"\xff\xd9", start + 2)
                if start < 0 or end < 0: break
                jpeg = buffer[start:end + 2]; header_blob = buffer[:start]; buffer = buffer[end + 2:]
                headers = {}
                for line in header_blob.replace(b"\r\n", b"\n").split(b"\n"):
                    if b":" in line:
                        key, value = line.split(b":", 1); headers[key.decode("ascii", "ignore").strip().lower()] = value.decode("ascii", "ignore").strip()
                image = cv2.imdecode(np.frombuffer(jpeg, np.uint8), cv2.IMREAD_COLOR)
                if image is None: continue
                valid = headers.get("x-weight-valid") == "1"
                yield StreamFrame(image, jpeg,
                                  float(headers["x-weight-grams"]) if valid and "x-weight-grams" in headers else None,
                                  int(headers["x-weight-raw"]) if valid and "x-weight-raw" in headers else None,
                                  int(headers["x-weight-age-ms"]) if valid and "x-weight-age-ms" in headers else None,
                                  valid, headers.get("x-weight-stable") == "1")


def frames(
    url: str,
    username: str | None = None,
    password: str | None = None,
    timeout: float = 10,
) -> Iterator[np.ndarray]:
    for packet in frames_with_metadata(url, username, password, timeout):
        yield packet.image
