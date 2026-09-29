# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

set(DOOM_SOURCES
    src/am_map.c
    src/d_event.c
    src/d_items.c
    src/d_iwad.c
    src/d_loop.c
    src/d_main.c
    src/d_mode.c
    src/doomdef.c
    src/doomgeneric.c
    src/doomstat.c
    src/dstrings.c
    src/f_finale.c
    src/f_wipe.c
    src/g_game.c
    src/hu_lib.c
    src/hu_stuff.c
    src/i_input.c
    src/i_sound.c
    src/i_system.c
    src/i_timer.c
    src/i_video.c
    src/info.c
    src/m_argv.c
    src/m_bbox.c
    src/m_cheat.c
    src/m_config.c
    src/m_controls.c
    src/m_fixed.c
    src/m_menu.c
    src/m_misc.c
    src/m_random.c
    src/memio.c
    src/mus2mid.c
    src/p_ceilng.c
    src/p_doors.c
    src/p_enemy.c
    src/p_floor.c
    src/p_inter.c
    src/p_lights.c
    src/p_map.c
    src/p_maputl.c
    src/p_mobj.c
    src/p_plats.c
    src/p_pspr.c
    src/p_saveg.c
    src/p_setup.c
    src/p_sight.c
    src/p_spec.c
    src/p_switch.c
    src/p_telept.c
    src/p_tick.c
    src/p_user.c
    src/r_bsp.c
    src/r_data.c
    src/r_draw.c
    src/r_main.c
    src/r_plane.c
    src/r_segs.c
    src/r_sky.c
    src/r_things.c
    src/s_sound.c
    src/sounds.c
    src/st_lib.c
    src/st_stuff.c
    src/tables.c
    src/v_video.c
    src/w_file.c
    src/w_main.c
    src/w_wad.c
    src/wi_stuff.c
    src/z_zone.c
)

idf_component_register(
    SRCS ${DOOM_SOURCES}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRED_IDF_TARGETS esp32s3
)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Build the engine with GCC.")
endif()

set_target_properties(${COMPONENT_LIB} PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
target_compile_features(${COMPONENT_LIB} PUBLIC c_std_23)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Os -ffunction-sections -fdata-sections -fno-strict-aliasing
    -Wall -Wextra -Wno-unused-parameter -Wno-error
)
target_link_libraries(${COMPONENT_LIB} PUBLIC m)
include(cmake/clang-tidy.cmake)
