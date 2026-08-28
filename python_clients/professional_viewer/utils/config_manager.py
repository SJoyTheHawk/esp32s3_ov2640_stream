#!/usr/bin/env python3
"""Configuration manager for professional camera viewer"""
import json
from pathlib import Path
from typing import Any


class ConfigManager:
    """Manages application configuration with JSON persistence"""

    def __init__(self, config_file: str = "config.json"):
        self.config_file = Path(config_file)
        self.config = self._load_config()

    def _load_config(self) -> dict:
        """Load configuration from file or return defaults"""
        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    return json.load(f)
            except (OSError, json.JSONDecodeError) as e:
                print(f"Error loading config: {e}, using defaults")

        # Return default configuration
        return self._get_default_config()

    def _get_default_config(self) -> dict:
        """Get default configuration"""
        return {
            "stream": {
                "url": "http://192.168.2.100/stream",
                "username": "admin",
                "password": "",
                "auto_reconnect": True,
                "reconnect_delay": 5
            },
            "capture": {
                "save_directory": "./captures",
                "burst_fps": 5,
                "photo_format": "jpg",
                "photo_quality": 95
            },
            "video": {
                "codec": "MJPEG",
                "fps": 20,
                "default_name": "recording"
            },
            "ui": {
                "theme": "dark",
                "window_geometry": [100, 100, 1000, 800],
                "show_fps": True,
                "show_resolution": True
            }
        }
    
    def save(self):
        """Save configuration to file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
        except OSError as e:
            print(f"Error saving config: {e}")
    
    def get(self, key: str, default: Any = None) -> Any:
        """Get configuration value using dot notation (e.g., 'stream.url')"""
        keys = key.split('.')
        value = self.config
        for k in keys:
            if isinstance(value, dict):
                value = value.get(k)
            else:
                return default
            if value is None:
                return default
        return value
    
    def set(self, key: str, value: Any):
        """Set configuration value using dot notation"""
        keys = key.split('.')
        config = self.config
        for k in keys[:-1]:
            if k not in config:
                config[k] = {}
            config = config[k]
        config[keys[-1]] = value
        self.save()