import unittest
from unittest.mock import Mock

from utils.device_api import DeviceApi, DeviceApiError


class FakeResponse:
    def __init__(self, status, payload):
        self.status_code = status
        self.payload = payload

    def json(self):
        return self.payload


class DeviceApiTests(unittest.TestCase):
    def client(self, responses):
        session = Mock()
        session.request.side_effect = [FakeResponse(200, {"status": "success"}), *responses]
        return DeviceApi("http://camera/stream", "admin", "secret", session), session

    def test_reads_led_state_after_login(self):
        api, session = self.client([FakeResponse(200, {"available": True, "enabled": True,
            "red": 1, "green": 2, "blue": 3, "brightness": 10})])
        api.login()
        self.assertEqual(api.get_led()["blue"], 3)
        self.assertEqual(session.request.call_count, 2)

    def test_sends_control_values(self):
        api, session = self.client([FakeResponse(200, {"status": "success"})])
        api.login()
        api.set_led({"enabled": False, "red": 1, "green": 2, "blue": 3, "brightness": 4})
        self.assertEqual(session.request.call_args.kwargs["data"],
                         {"enabled": "false", "red": 1, "green": 2, "blue": 3, "brightness": 4})

    def test_authentication_failure_is_reported(self):
        session = Mock(); session.request.return_value = FakeResponse(401, {"status": "error", "message": "Unauthorized"})
        with self.assertRaisesRegex(DeviceApiError, "Authentication failed"):
            DeviceApi("http://camera/stream", "admin", "bad", session).login()

    def test_invalid_values_are_rejected_before_request(self):
        api, session = self.client([])
        with self.assertRaises(DeviceApiError):
            api.set_led({"enabled": True, "red": 256, "green": 0, "blue": 0, "brightness": 10})
        self.assertEqual(session.request.call_count, 0)

    def test_unreachable_device_is_reported(self):
        session = Mock(); session.request.side_effect = OSError("offline")
        # requests.Session normally raises RequestException; use a fake that does so.
        import requests
        session.request.side_effect = requests.RequestException("offline")
        with self.assertRaisesRegex(DeviceApiError, "Cannot reach device"):
            DeviceApi("http://camera/stream", "admin", "secret", session).login()


if __name__ == "__main__":
    unittest.main()
