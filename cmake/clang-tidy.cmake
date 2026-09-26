find_program(DOOM_CLANG_TIDY NAMES clang-tidy
    HINTS "/Applications/CLion.app/Contents/bin/clang/mac/aarch64/bin"
    DOC "Clang-tidy executable with Xtensa support"
)

if(DOOM_CLANG_TIDY)
    # Clang needs the GCC headers, but does not accept GCC's -mlongcalls.
    # Pass analysis arguments explicitly; the compilation database stays intact.
    set(_doom_tidy_args --target=xtensa-esp32s3-elf -std=gnu99
        "-I${CMAKE_CURRENT_SOURCE_DIR}/src")
    foreach(_doom_include IN LISTS CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
        list(APPEND _doom_tidy_args -isystem "${_doom_include}")
    endforeach()

    add_custom_target(clang-tidy
        COMMAND "${DOOM_CLANG_TIDY}" ${DOOM_SOURCES} -- ${_doom_tidy_args}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Analyzing the engine with clang-tidy"
        VERBATIM
    )
endif()
