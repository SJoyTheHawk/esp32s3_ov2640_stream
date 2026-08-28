#!/bin/bash
# Quick test launcher for professional viewer

cd "$(dirname "$0")"

echo "ESP32 Camera Viewer Pro - Test Launcher"
echo "========================================="
echo ""

# Check Python version
python3 --version

# Check if dependencies are installed
echo ""
echo "Checking dependencies..."
python3 -c "import PyQt6; print('✓ PyQt6 installed')" 2>/dev/null || echo "✗ PyQt6 not installed - run: pip install -r requirements.txt"
python3 -c "import cv2; print('✓ OpenCV installed')" 2>/dev/null || echo "✗ OpenCV not installed - run: pip install -r requirements.txt"
python3 -c "import numpy; print('✓ NumPy installed')" 2>/dev/null || echo "✗ NumPy not installed - run: pip install -r requirements.txt"

echo ""
echo "Starting application..."
echo ""

# Run the application
python3 camera_viewer_pro.py
