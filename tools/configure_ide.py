# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

"""Capture the activated ESP-IDF environment in a local CMake/CLion preset.

Run with ESP-IDF's Python after sourcing export.sh. No machine paths are stored in Git.
"""

import json
import os
from pathlib import Path
import sys


def main():
    root = Path(__file__).resolve().parent.parent
    idf_path = os.environ.get("IDF_PATH", "")
    if not idf_path or not (Path(idf_path) / "tools/cmake/project.cmake").is_file():
        sys.exit("Source your ESP-IDF installation's export.sh, then run python tools/configure_ide.py.")
    python_env = os.environ.get("IDF_PYTHON_ENV_PATH")
    if not python_env or Path(sys.prefix).resolve() != Path(python_env).resolve():
        sys.exit("Source ESP-IDF's export.sh and use its Python to run tools/configure_ide.py.")

    # Only build-related values; never copy the entire shell environment.
    keys = ("PATH", "IDF_PATH", "IDF_TOOLS_PATH", "IDF_PYTHON_ENV_PATH", "ESP_ROM_ELF_DIR")
    environment = {key: os.environ[key] for key in keys if key in os.environ}
    preset = {
        "name": "esp32s3-clion",
        "displayName": "ESP32-S3 (local ESP-IDF environment)",
        "inherits": "esp32s3",
        "binaryDir": "build-clion",
        "environment": environment,
        "cacheVariables": {"PYTHON": sys.executable},
    }
    path = root / "CMakeUserPresets.json"
    data = json.loads(path.read_text()) if path.exists() else {"version": 3}
    presets = data.setdefault("configurePresets", [])
    presets[:] = [item for item in presets if item["name"] != preset["name"]]
    presets.append(preset)
    builds = data.setdefault("buildPresets", [])
    builds[:] = [item for item in builds if item["name"] != preset["name"]]
    builds.append({"name": preset["name"], "configurePreset": preset["name"]})
    path.write_text(json.dumps(data, indent=2) + "\n")
    print(f"Wrote {path}")
    print("Enable the esp32s3-clion CMake profile, then Reset Cache and Reload Project.")
    print("Regenerate this preset whenever your ESP-IDF installation changes.")


if __name__ == "__main__":
    main()
