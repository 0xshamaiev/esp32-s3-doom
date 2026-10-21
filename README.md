# DOOM on TinyS3

This is my port of the classic DOOM engine for the TinyS3 ESP32-S3 development
board. The hardware setup pairs the board with a 2.0-inch, 320 × 240 ST7789V
LCD (GMT020-02) and an i8 mini keyboard. The goal is a small, keyboard-controlled DOOM system
built with ESP-IDF.

The engine has been trimmed down for this project, with a focus on single-player
DOOM and DOOM II. Mouse input and network multiplayer have been removed. The
framebuffer is DOOM's native 320 × 200, centered on the display with black bars
above and below.

## Hardware

| Part | Role |
| --- | --- |
| TinyS3 ESP32-S3 development board | Runs the firmware and game engine |
| GMT020-02 2.0-inch, 320 × 240 ST7789V LCD | Displays the game |
| i8 mini keyboard | Provides keyboard controls over USB OTG |

Controls are intended to use keys only. The TinyS3 can't supply 5V on its USB-C
port, so the keyboard needs an OTG adapter or cable that powers it.

## Current status

The firmware loads `doom1.wad` from SPIFFS and runs DOOM on the LCD.
The game framebuffer and zone memory both live in PSRAM.

Still missing: audio. Mouse input and multiplayer are removed.
WAD merging is disabled too.

The engine supports DOOM and DOOM II. It looks for `doom.wad`, `doom1.wad`, or
`doom2.wad`, but startup currently hardcodes `/spiffs/doom1.wad`.
Game data isn't included.

## Build and flash

Install ESP-IDF and its ESP32-S3 tools first. This project has been built with
ESP-IDF 6.1. From the repository root, activate your installation and build:

```sh
. /path/to/esp-idf/export.sh
idf.py build
```

Replace `/path/to/esp-idf` with your ESP-IDF installation directory. The project
selects ESP32-S3 by default and writes build artifacts to `build/`.

Put your WAD at `spiffs/doom1.wad` before building. CMake packs that folder into
an image for the `storage` partition. `spiffs/doom*` is ignored by Git.

To flash:

```sh
idf.py -p YOUR_PORT flash
```

Use your board's serial port in place of `YOUR_PORT`. Once the game starts, the
USB-C port works as a USB host for the keyboard and disappears from your computer.
To flash again, hold BOOT, tap RESET, then release BOOT. Logs go to UART0 on
GPIO 43 (TX) and 44 (RX).

## Repository layout

```text
main/                     Application startup and board integration
components/doom/
  include/                Public engine API and key codes
  src/                    Engine implementation and private headers
  cmake/                  Optional engine analysis tooling
docs/                     Documentation and notes
tools/configure_ide.py    Generate a local IDE environment preset
CMakeLists.txt            ESP-IDF application initialization
CMakePresets.json         Shared ESP32-S3 firmware preset
```

[Build and CLion notes](docs/development.md).
[How to swap the display driver](docs/display.md).

## License and credits

The original TinyS3 application, tooling, and documentation are licensed under
the **GNU GPL version 3 or later**; see [LICENSE](LICENSE). The inherited DOOM
engine retains its upstream notices, and this port's changes to that engine
(apart from the rewritten configuration header) are offered under GPL version 2
or later. This preserves attribution and the engine's existing license options.

See [the licensing notes](docs/licensing.md) for the scope of these licenses.

This port builds on id Software's DOOM engine, Chocolate Doom, DoomGeneric, and
the work of many other contributors. See [AUTHORS.md](AUTHORS.md) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Original authors retain credit
for their work regardless of how extensively this port changes it.

The software is provided **without warranty**, with liability limited as stated
in the applicable licenses and to the extent permitted by law. Game data is not
included and is not covered by the engine's software license.
