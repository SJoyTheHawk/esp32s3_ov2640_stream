"""Authenticated LED API. Device state is never cached in local configuration."""
from urllib.parse import urlsplit, urlunsplit

import requests


class DeviceApiError(RuntimeError):
    pass


class DeviceApi:
    def __init__(self, stream_url, username, password, session=None):
        parts = urlsplit(stream_url.strip())
        if parts.scheme not in ("http", "https") or not parts.hostname or parts.username:
            raise DeviceApiError("Enter an HTTP(S) stream URL without embedded credentials.")
        self.base_url = urlunsplit((parts.scheme, parts.netloc, "", "", ""))
        self.username = username
        self.password = password
        self.session = session if session is not None else requests.Session()

    def close(self):
        self.session.close()

    def _request(self, method, path, data=None):
        try:
            response = self.session.request(method, self.base_url + path, data=data,
                                            timeout=(1, 2), allow_redirects=False)
            try:
                payload = response.json()
            except ValueError as exc:
                raise DeviceApiError("Device returned an invalid response. Check firmware support.") from exc
            if not isinstance(payload, dict):
                raise DeviceApiError("Device returned an invalid response.")
            if response.status_code == 401:
                raise DeviceApiError("Authentication failed. Check username/password or reload to log in again.")
            if not 200 <= response.status_code < 300:
                raise DeviceApiError(payload.get("message") or f"Device request failed ({response.status_code}).")
            if payload.get("status") == "error":
                raise DeviceApiError(payload.get("message", "Device request failed."))
            return payload
        except requests.RequestException as exc:
            raise DeviceApiError("Cannot reach device. Check the connection; reload to confirm its state.") from exc

    def login(self):
        result = self._request("POST", "/api/login", {"username": self.username, "password": self.password})
        if result.get("status") != "success":
            raise DeviceApiError("Device login failed.")

    @staticmethod
    def validate_controls(values):
        if type(values.get("enabled")) is not bool:
            raise DeviceApiError("LED enabled must be a boolean.")
        for key, maximum in (("red", 255), ("green", 255), ("blue", 255), ("brightness", 100)):
            if type(values.get(key)) is not int or not 0 <= values[key] <= maximum:
                raise DeviceApiError(f"LED {key} must be an integer from 0 to {maximum}.")

    def get_led(self):
        result = self._request("GET", "/api/led")
        if result.get("available") is not True:
            raise DeviceApiError("LED controller unavailable.")
        self.validate_controls(result)
        return result

    def set_led(self, values):
        self.validate_controls(values)
        body = {key: values[key] for key in ("red", "green", "blue", "brightness")}
        body["enabled"] = str(values["enabled"]).lower()
        return self._request("POST", "/api/led/control", body)
