"""Perform bounded LED API requests away from the Qt event loop."""
from PyQt6.QtCore import QThread
from utils.device_api import DeviceApi, DeviceApiError


class LedWorker(QThread):
    def __init__(self, connection, values=None, parent=None):
        super().__init__(parent)
        self.connection = connection
        self.values = values
        self.result = None
        self.error = None

    def run(self):
        client = None
        try:
            client = DeviceApi(*self.connection)
            client.login()
            if self.values is None:
                self.result = client.get_led()
            else:
                self.result = client.set_led(self.values)
        except (DeviceApiError, ValueError) as exc:
            self.error = str(exc)
        finally:
            if client is not None:
                client.close()
