# Development notes

## Build setup

The root CMake file sets up ESP-IDF. `main/` has the app and hardware code.
`components/doom/` has the engine, its source list, and its C23 settings.

The engine exposes `include/`. Use `doomgeneric.h` from the app, and
`doomkeys.h` when adding controls. The other engine headers are private.

ESP-IDF handles the compiler, linker, bootloader, partition table, and firmware
image. There's no separate engine build.

## CLion

If you open CLion from the Dock, it might not have the ESP-IDF environment.
Running `export.sh` in CLion's terminal only fixes that terminal. CMake still
needs its own environment.

Here's the setup:

1. Open the project root in CLion.
2. Run this in a terminal at the project root:

   ```sh
   . /path/to/esp-idf/export.sh
   python tools/configure_ide.py
   ```

   Use your actual ESP-IDF path. If you already have an alias like `get_idf`,
   that works too.

3. In **Settings → Build, Execution, Deployment → CMake**, enable
   **esp32s3-clion**. Disable the old **Debug** profile and any unused ones.
4. Run **Tools → CMake → Reset Cache and Reload Project**.
5. Build the **app** target. Flash with `idf.py` from a terminal, or use the
   **flash** target once its serial port is set.

The script writes `CMakeUserPresets.json`. It's local and ignored by Git.
Run the script again if you move ESP-IDF or switch versions. It keeps your
other presets.

CLion builds in `build-clion/`. Terminal builds normally go in `build/`.
Both use the same `sdkconfig`, so don't change it from both builds at once.
Make sure you're flashing from the folder you actually rebuilt.

The local preset also works outside CLion, without sourcing `export.sh` first:

```sh
cmake --preset esp32s3-clion
cmake --build --preset esp32s3-clion
```

Use the build preset so the ESP-IDF environment is also available to build-time
steps such as bootloader configuration.

Run the optional `clang-tidy` target to analyze both the engine and app:

```sh
cmake --build --preset esp32s3-clion --target clang-tidy
```

The target requires clang-tidy with Xtensa and C23 support. Set
`DOOM_CLANG_TIDY` in your local CMake preset to select a different executable.
Analysis uses the components' firmware include paths and definitions, with the
GCC system headers from the configured toolchain.

Another option is to set your ESP-IDF `export.sh` as the CLion toolchain's
environment file and use the **esp32s3** preset.
[JetBrains has a setup guide](https://www.jetbrains.com/help/clion/esp-idf.html).

### “This file doesn't belong to any project”

Usually CMake failed to load, or CLion is using the wrong profile.
Check the CMake errors, enable the ESP-IDF profile, then reset the cache and
reload. The old standalone **Debug** profile won't work with this build.

App files should show up in `compile_commands.json` under `__idf_main`.
Engine files belong to `__idf_doom`. Fix the CMake error rather than manually
marking folders as source directories.

## App startup

`main/main.c` reserves the framebuffer early, mounts storage, and starts the LCD.
Then it calls `doomgeneric_Create()` and keeps calling `doomgeneric_Tick()`.
The loop delays for one RTOS tick so other tasks get CPU time.

The framebuffer stays in internal SRAM. Zone memory uses PSRAM.
`main/doomgeneric_esp32.c` has the timing and drawing hooks; input is still a stub.

Keep hardware code in its own files. The display already has a small interface,
so swapping it doesn't require changing the Doom hooks.
See [the display notes](display.md).
