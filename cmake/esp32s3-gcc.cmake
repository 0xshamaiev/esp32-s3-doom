set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# An optional installation root for toolchains outside ESP-IDF's tools folder.
set(DOOM_GCC_ROOT "" CACHE PATH "Xtensa GCC installation containing bin/")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES DOOM_GCC_ROOT DOOM_GCC_COMPILER)

if(DEFINED ENV{IDF_TOOLS_PATH})
    set(_doom_idf_tools "$ENV{IDF_TOOLS_PATH}")
else()
    set(_doom_idf_tools "$ENV{HOME}/.espressif")
endif()

# GUI applications do not necessarily inherit the terminal's exported PATH.
file(GLOB _doom_gcc_paths LIST_DIRECTORIES TRUE
    "${_doom_idf_tools}/tools/xtensa-esp-elf/*/xtensa-esp-elf/bin"
    "${_doom_idf_tools}/tools/xtensa-esp32s3-elf/*/xtensa-esp32s3-elf/bin"
)
list(SORT _doom_gcc_paths COMPARE NATURAL ORDER DESCENDING)

set(_doom_gcc_hints)
if(DOOM_GCC_ROOT)
    list(APPEND _doom_gcc_hints "${DOOM_GCC_ROOT}/bin")
endif()
find_program(DOOM_GCC_COMPILER NAMES xtensa-esp32s3-elf-gcc
    HINTS ${_doom_gcc_hints}
    PATHS ${_doom_gcc_paths}
)
if(NOT DOOM_GCC_COMPILER)
    message(FATAL_ERROR
        "ESP32-S3 GCC was not found. Install the ESP-IDF Xtensa toolchain, "
        "or set DOOM_GCC_ROOT to its installation directory (containing bin/).")
endif()

get_filename_component(_doom_gcc_bin "${DOOM_GCC_COMPILER}" DIRECTORY)
set(CMAKE_C_COMPILER "${DOOM_GCC_COMPILER}")
set(CMAKE_AR "${_doom_gcc_bin}/xtensa-esp32s3-elf-ar")
set(CMAKE_RANLIB "${_doom_gcc_bin}/xtensa-esp32s3-elf-ranlib")
set(CMAKE_C_FLAGS_INIT "-mlongcalls")
