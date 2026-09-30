# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

find_program(DOOM_CLANG_TIDY NAMES clang-tidy
    HINTS "/Applications/CLion.app/Contents/bin/clang/mac/aarch64/bin"
    DOC "Clang-tidy executable with Xtensa support"
)

if(DOOM_CLANG_TIDY)
    # Clang needs the GCC headers, but does not accept GCC's -mlongcalls.
    # Pass analysis arguments explicitly; the compilation database stays intact.
    set(_doom_tidy_args --target=xtensa-esp32s3-elf -std=gnu23)
    foreach(_doom_include IN LISTS CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
        list(APPEND _doom_tidy_args -isystem "${_doom_include}")
    endforeach()

    add_custom_target(clang-tidy
        COMMAND "${CMAKE_COMMAND}" -E chdir "${CMAKE_CURRENT_SOURCE_DIR}"
            "${DOOM_CLANG_TIDY}" ${DOOM_SOURCES} -- ${_doom_tidy_args}
            "-I$<JOIN:$<TARGET_PROPERTY:__idf_doom,INCLUDE_DIRECTORIES>,;-I>"
            "-D$<JOIN:$<TARGET_PROPERTY:__idf_doom,COMPILE_DEFINITIONS>,;-D>"
        COMMAND "${CMAKE_COMMAND}" -E chdir "${CMAKE_SOURCE_DIR}/main"
            "${DOOM_CLANG_TIDY}" "$<TARGET_PROPERTY:__idf_main,SOURCES>" -- ${_doom_tidy_args}
            "-I$<JOIN:$<TARGET_PROPERTY:__idf_main,INCLUDE_DIRECTORIES>,;-I>"
            "-D$<JOIN:$<TARGET_PROPERTY:__idf_main,COMPILE_DEFINITIONS>,;-D>"
        COMMENT "Analyzing the engine and app with clang-tidy"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endif()
