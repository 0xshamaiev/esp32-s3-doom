# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR
        "ESP-IDF is not activated. Source your installation's export.sh before idf.py or CMake. "
        "For CLion, see docs/development.md.")
endif()

set(IDF_TARGET "esp32s3" CACHE STRING "ESP-IDF target")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(esp32_s3_doom)
