"""Authenticated multipart MJPEG reader for the professional viewer."""

from typing import Iterator

import cv2
import numpy as np
import requests


def frames(
    url: str,
    username: str | None = None,
    password: str | None = None,
    timeout: float = 10,
) -> Iterator[np.ndarray]:
    session = requests.Session()
    if username is not None and password is not None:
        response = session.post(
            url.rsplit("/", 1)[0] + "/api/login",
            data={"username": username, "password": password},
            timeout=timeout,
        )
        response.raise_for_status()
        if response.json().get("status") != "success":
            raise RuntimeError("ESP32 login failed")

    with session.get(url, stream=True, timeout=(timeout, timeout)) as response:
        response.raise_for_status()
        buffer = b""
        for chunk in response.iter_content(chunk_size=8192):
            buffer += chunk
            while True:
                start = buffer.find(b"\xff\xd8")
                end = buffer.find(b"\xff\xd9", start + 2)
                if start < 0 or end < 0:
                    break
                image = cv2.imdecode(
                    np.frombuffer(buffer[start:end + 2], np.uint8),
                    cv2.IMREAD_COLOR,
                )
                buffer = buffer[end + 2:]
                if image is not None:
                    yield image
