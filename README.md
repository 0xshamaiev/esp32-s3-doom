# DOOM on TinyS3

This is my port of the classic DOOM engine for the TinyS3 ESP32-S3 development
board. The hardware setup pairs the board with a Waveshare 1.14-inch, 240 × 135
LCD and an i8 mini keyboard. The goal is a small, keyboard-controlled DOOM system
built with ESP-IDF.

The engine has been trimmed down for this project, with a focus on single-player
DOOM and DOOM II. Mouse input and network multiplayer have been removed, and the
framebuffer is configured for the display's 240 × 135 resolution.

## Hardware

| Part | Role |
| --- | --- |
| TinyS3 ESP32-S3 development board | Runs the firmware and game engine |
| Waveshare 1.14-inch, 240 × 135 LCD | Displays the game |
| i8 mini keyboard | Provides keyboard controls |

Controls are intended to use keys only.

## Current status

This is a work in progress. The engine compiles as an ESP-IDF component, and the
project builds a firmware image with a bootloader and partition table. The current
application entry point only logs startup readiness; it does not launch DOOM yet.
Display and keyboard drivers, WAD storage, and board memory configuration still
need to be connected to the engine.

The current scope and limitations are:

- **Single-player only:** no network multiplayer.
- **Keyboard input:** no mouse support.
- **DOOM and DOOM II:** IWAD discovery is limited to `doom.wad`, `doom1.wad`
  (shareware), and `doom2.wad`.
- **No audio output yet:** sound effects and music have no active platform backend.
- **No DeHackEd patches or WAD merging:** these features are disabled.

Game data is not included. Running the game will require a compatible IWAD—the
file containing its maps, graphics, and other assets—on storage accessible to the
board.

## Build and flash

Install ESP-IDF and its ESP32-S3 tools first. This project has been built with
ESP-IDF 6.1. From the repository root, activate your installation and build:

```sh
. /path/to/esp-idf/export.sh
idf.py build
```

Replace `/path/to/esp-idf` with your ESP-IDF installation directory. The project
selects ESP32-S3 by default and writes build artifacts to `build/`.

To flash the current startup firmware and view its serial output:

```sh
idf.py -p YOUR_PORT flash monitor
```

Replace `YOUR_PORT` with the board's serial port. Exit the monitor with `Ctrl+]`.
Flashing this version provides the startup application described above; gameplay
integration is still in progress.

## Repository layout

```text
main/                     Application startup and board integration
components/doom/
  include/                Public engine API and key codes
  src/                    Engine implementation and private headers
  cmake/                  Optional engine analysis tooling
docs/development.md       Build and CLion setup
tools/configure_ide.py    Generate a local IDE environment preset
CMakeLists.txt            ESP-IDF application initialization
CMakePresets.json         Shared ESP32-S3 firmware preset
```

For CLion configuration and notes on implementing the board support, see the
[development guide](docs/development.md).

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
