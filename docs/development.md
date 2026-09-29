# Development

## Components

The root CMake file initializes ESP-IDF. `main` explicitly depends on `doom` and
`log`. The engine component owns its complete source list, private headers, public
headers, and C23 compilation settings. It exports only `include/`; application
code should include `doomgeneric.h` and, when implementing controls, `doomkeys.h`.

ESP-IDF owns the compiler, linker scripts, bootloader, partition table, and binary
generation. There is no separate standalone engine toolchain or build mode.

## CLion

CLion launched from the Dock does not necessarily inherit the ESP-IDF environment
from an interactive terminal. Sourcing ESP-IDF's `export.sh` in CLion's terminal
activates that terminal only; it does not update the CMake process environment.

This repository provides a local preset to give CMake the activated environment:

1. Open the repository root as a CMake project.
2. In an interactive terminal at the repository root, run:

   ```sh
   . /path/to/esp-idf/export.sh
   python tools/configure_ide.py
   ```

   Replace `/path/to/esp-idf` with your ESP-IDF installation directory.
   A personal shell alias such as `get_idf` can be used as a shortcut for sourcing
   `export.sh`; the alias is not supplied by ESP-IDF or required by this project.

3. In **Settings → Build, Execution, Deployment → CMake**, enable
   **esp32s3-clion** and disable the old **Debug** profile and unused profiles.
4. Use **Tools → CMake → Reset Cache and Reload Project**.
5. Build the **app** target. Flash from the terminal with `idf.py`, or use the
   **flash** target after configuring the appropriate serial port.

`CMakeUserPresets.json` is ignored by Git and contains only local build paths and
the selected ESP-IDF environment. Regenerate it after changing ESP-IDF versions or
installation paths. Other user presets are preserved. The IDE uses `build-clion/`,
separate from the terminal's `build/`, while both use the project's `sdkconfig`.
Avoid changing `sdkconfig` from two builds at the same time.

You can also test the local preset outside CLion without activating the shell:

```sh
cmake --preset esp32s3-clion
cmake --build --preset esp32s3-clion
```

Use the build preset so the ESP-IDF environment is also available to build-time
steps such as bootloader configuration.

Alternatively, configure a CLion toolchain environment file pointing at your
ESP-IDF installation's `export.sh`, and use the shared **esp32s3** preset.
See [JetBrains' ESP-IDF setup guide](https://www.jetbrains.com/help/clion/esp-idf.html).

### “This file doesn't belong to any project”

This means CLion has no successfully loaded target containing that file. Both
`main/main.c` and the engine sources are registered with ESP-IDF. Check the CMake
output for configuration errors, ensure the ESP-IDF profile is enabled, then
reset its cache and reload. A stale standalone `Debug` profile cannot supply the
new firmware project model.

The file should appear in the active profile's `compile_commands.json` and the
CMake target `__idf_main`. Do not mark the source directory manually to conceal a
failed CMake configuration.

## Board integration

Keep application and hardware startup code in `main/`. When drivers become
substantial, give them their own components and explicit dependencies.

Before calling `doomgeneric_Create()`, implement `DG_Init`, `DG_DrawFrame`,
`DG_SleepMs`, `DG_GetTicksMs`, and `DG_GetKey`; mount WAD storage; and configure
sufficient memory. The current engine zone allocation requests 6–8 MiB by
default. Call `doomgeneric_Tick()` repeatedly from an appropriately sized and
scheduled task once initialization succeeds.
