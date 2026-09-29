# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

idf_component_register(
    SRCS "main.c"
    REQUIRES esp_psram spiffs driver
    PRIV_REQUIRES doom log
)
