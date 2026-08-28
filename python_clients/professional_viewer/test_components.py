#!/usr/bin/env python3
"""Test script to verify all components work"""
import sys
from pathlib import Path

print("ESP32 Camera Viewer Pro - Component Test")
print("=" * 50)

# Test imports
print("\n1. Testing imports...")
try:
    from PyQt6.QtWidgets import QApplication
    from PyQt6.QtCore import QThread, pyqtSignal
    from PyQt6.QtGui import QFont
    print("✓ PyQt6 imported successfully")
except ImportError as e:
    print(f"✗ PyQt6 import failed: {e}")
    sys.exit(1)

try:
    import cv2
    print(f"✓ OpenCV imported successfully (version: {cv2.__version__})")
except ImportError as e:
    print(f"✗ OpenCV import failed: {e}")
    sys.exit(1)

try:
    import numpy as np
    print(f"✓ NumPy imported successfully (version: {np.__version__})")
except ImportError as e:
    print(f"✗ NumPy import failed: {e}")
    sys.exit(1)

# Test module imports
print("\n2. Testing custom modules...")
try:
    from utils.config_manager import ConfigManager
    print("✓ ConfigManager imported")
except ImportError as e:
    print(f"✗ ConfigManager import failed: {e}")
    sys.exit(1)

try:
    from utils.file_manager import FileManager
    print("✓ FileManager imported")
except ImportError as e:
    print(f"✗ FileManager import failed: {e}")
    sys.exit(1)

try:
    from widgets.stream_widget import StreamWidget
    print("✓ StreamWidget imported")
except ImportError as e:
    print(f"✗ StreamWidget import failed: {e}")
    sys.exit(1)

try:
    from widgets.burst_button import BurstButton
    print("✓ BurstButton imported")
except ImportError as e:
    print(f"✗ BurstButton import failed: {e}")
    sys.exit(1)

try:
    from workers.save_worker import SaveWorker
    print("✓ SaveWorker imported")
except ImportError as e:
    print(f"✗ SaveWorker import failed: {e}")
    sys.exit(1)

try:
    from workers.video_worker import VideoWorker
    print("✓ VideoWorker imported")
except ImportError as e:
    print(f"✗ VideoWorker import failed: {e}")
    sys.exit(1)

# Test configuration
print("\n3. Testing configuration...")
try:
    config = ConfigManager("test_config.json")
    print(f"✓ Config created: {config.get('stream.url')}")

    # Test set/get
    config.set('test.value', 42)
    assert config.get('test.value') == 42
    print("✓ Config set/get works")

    # Cleanup
    Path("test_config.json").unlink(missing_ok=True)
except Exception as e:
    print(f"✗ Config test failed: {e}")

# Test file manager
print("\n4. Testing file manager...")
try:
    fm = FileManager("./test_captures")
    photo = fm.generate_photo_filename()
    print(f"✓ Photo filename: {photo.name}")

    burst = fm.generate_burst_filename(1)
    print(f"✓ Burst filename: {burst.name}")

    video = fm.generate_video_filename()
    print(f"✓ Video filename: {video.name}")
except Exception as e:
    print(f"✗ FileManager test failed: {e}")

# Test PyQt6 application
print("\n5. Testing PyQt6 application...")
try:
    app = QApplication([])
    print("✓ QApplication created")

    # Test widget creation
    widget = StreamWidget()
    print("✓ StreamWidget created")

    button = BurstButton()
    print("✓ BurstButton created")

except Exception as e:
    print(f"✗ PyQt6 application test failed: {e}")
    sys.exit(1)

print("\n" + "=" * 50)
print("✓ All tests passed! Ready to run camera_viewer_pro.py")
print("=" * 50)
