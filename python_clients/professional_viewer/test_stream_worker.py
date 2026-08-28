#!/usr/bin/env python3
"""Tests for stream reconnection behavior."""
import unittest
from unittest.mock import patch

import numpy as np

from workers.stream_worker import StreamWorker


class StreamWorkerTest(unittest.TestCase):
    def test_reconnects_after_connection_failure(self):
        worker = StreamWorker("http://camera/stream", reconnect_delay=0)
        attempts = []
        connected = []
        errors = []

        def fake_frames(*_args):
            attempts.append(None)
            if len(attempts) == 1:
                raise ConnectionError("camera unavailable")
            yield np.zeros((1, 1, 3), dtype=np.uint8)

        worker.connected_signal.connect(lambda: connected.append(None))
        worker.frame_ready.connect(lambda _frame: worker.stop())
        worker.error_signal.connect(errors.append)

        with patch("workers.stream_worker.frames", fake_frames):
            worker.run()

        self.assertEqual(len(attempts), 2)
        self.assertEqual(len(connected), 1)
        self.assertIn("Connection failed", errors[0])

    def test_stops_after_failure_when_reconnect_is_disabled(self):
        worker = StreamWorker("http://camera/stream", auto_reconnect=False)
        attempts = []

        def fake_frames(*_args):
            attempts.append(None)
            raise ConnectionError("camera unavailable")
            yield

        with patch("workers.stream_worker.frames", fake_frames):
            worker.run()

        self.assertEqual(len(attempts), 1)

    def test_reconnects_after_stream_ends(self):
        worker = StreamWorker("http://camera/stream", reconnect_delay=0)
        attempts = []
        frames_received = []

        def fake_frames(*_args):
            attempts.append(None)
            yield np.zeros((1, 1, 3), dtype=np.uint8)

        def receive_frame(_frame):
            frames_received.append(None)
            if len(frames_received) == 2:
                worker.stop()

        worker.frame_ready.connect(receive_frame)

        with patch("workers.stream_worker.frames", fake_frames):
            worker.run()

        self.assertEqual(len(attempts), 2)
        self.assertEqual(len(frames_received), 2)


if __name__ == "__main__":
    unittest.main()
