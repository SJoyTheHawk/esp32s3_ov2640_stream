#!/usr/bin/env python3
"""Upload captured JPEG files to an HTTP multipart endpoint."""
import argparse
from pathlib import Path
import requests

def upload(directory: Path, endpoint: str, delete: bool = False) -> None:
    for path in sorted(directory.glob("*.jp*g")):
        try:
            with path.open("rb") as image:
                response = requests.post(endpoint, files={"file": (path.name, image, "image/jpeg")}, timeout=30)
            response.raise_for_status(); print(f"Uploaded {path}")
            if delete: path.unlink()
        except (OSError, requests.RequestException) as error:
            print(f"Failed {path}: {error}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("--directory", type=Path, default=Path("captures"))
    parser.add_argument("--url", required=True); parser.add_argument("--delete", action="store_true")
    args = parser.parse_args(); upload(args.directory, args.url, args.delete)
