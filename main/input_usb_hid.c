// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "input.h"
#include "hid_report.h"
#include "doomkeys.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/usb_host.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "usb_kbd";

typedef struct {
    uint8_t pressed;
    uint8_t key;
} key_event_t;

static QueueHandle_t s_key_queue;
static QueueHandle_t s_connect_queue;

static constexpr uint8_t hid_to_doom[] = {
    [HID_KEY_ENTER] = KEY_ENTER,
    [HID_KEY_ESC] = KEY_ESCAPE,
    [HID_KEY_DEL] = KEY_BACKSPACE,
    [HID_KEY_TAB] = KEY_TAB,
    [HID_KEY_SPACE] = KEY_USE,
    [HID_KEY_MINUS] = KEY_MINUS,
    [HID_KEY_EQUAL] = KEY_EQUALS,
    [HID_KEY_OPEN_BRACKET] = '[',
    [HID_KEY_CLOSE_BRACKET] = ']',
    [HID_KEY_BACK_SLASH] = '\\',
    [HID_KEY_COLON] = ';',
    [HID_KEY_QUOTE] = '\'',
    [HID_KEY_TILDE] = '`',
    [HID_KEY_LESS] = ',',
    [HID_KEY_GREATER] = '.',
    [HID_KEY_SLASH] = '/',
    [HID_KEY_CAPS_LOCK] = KEY_CAPSLOCK,
    [HID_KEY_F11] = KEY_F11,
    [HID_KEY_F12] = KEY_F12,
    [HID_KEY_PRINT_SCREEN] = KEY_PRTSCR,
    [HID_KEY_SCROLL_LOCK] = KEY_SCRLCK,
    [HID_KEY_PAUSE] = KEY_PAUSE,
    [HID_KEY_INSERT] = KEY_INS,
    [HID_KEY_HOME] = KEY_HOME,
    [HID_KEY_PAGEUP] = KEY_PGUP,
    [HID_KEY_DELETE] = KEY_DEL,
    [HID_KEY_END] = KEY_END,
    [HID_KEY_PAGEDOWN] = KEY_PGDN,
    [HID_KEY_RIGHT] = KEY_RIGHTARROW,
    [HID_KEY_LEFT] = KEY_LEFTARROW,
    [HID_KEY_DOWN] = KEY_DOWNARROW,
    [HID_KEY_UP] = KEY_UPARROW,
    [HID_KEY_NUM_LOCK] = KEY_NUMLOCK,
    [HID_KEY_KEYPAD_DIV] = KEYP_DIVIDE,
    [HID_KEY_KEYPAD_MUL] = KEYP_MULTIPLY,
    [HID_KEY_KEYPAD_SUB] = KEYP_MINUS,
    [HID_KEY_KEYPAD_ADD] = KEYP_PLUS,
    [HID_KEY_KEYPAD_ENTER] = KEYP_ENTER,
    [HID_KEY_KEYPAD_1] = KEYP_1,
    [HID_KEY_KEYPAD_2] = KEYP_2,
    [HID_KEY_KEYPAD_3] = KEYP_3,
    [HID_KEY_KEYPAD_4] = KEYP_4,
    [HID_KEY_KEYPAD_5] = KEYP_5,
    [HID_KEY_KEYPAD_6] = KEYP_6,
    [HID_KEY_KEYPAD_7] = KEYP_7,
    [HID_KEY_KEYPAD_8] = KEYP_8,
    [HID_KEY_KEYPAD_9] = KEYP_9,
};

static uint8_t translate_usage(uint8_t usage) {
    if (usage >= HID_KEY_A && usage <= HID_KEY_Z)
        return 'a' + (usage - HID_KEY_A);
    if (usage >= HID_KEY_1 && usage < HID_KEY_0)
        return '1' + (usage - HID_KEY_1);
    if (usage == HID_KEY_0)
        return '0';
    if (usage >= HID_KEY_F1 && usage <= HID_KEY_F10)
        return KEY_F1 + (usage - HID_KEY_F1);
    if (usage >= HID_KEY_LEFT_CONTROL && usage <= HID_KEY_LEFT_CONTROL + 7) {
        static const uint8_t modifiers[] = {KEY_FIRE, KEY_RSHIFT, KEY_RALT, 0};
        return modifiers[(usage - HID_KEY_LEFT_CONTROL) & 3];
    }
    if (usage < sizeof(hid_to_doom))
        return hid_to_doom[usage];
    return 0;
}

static void post_key(bool pressed, uint8_t key) {
    const key_event_t event = {.pressed = pressed, .key = key};
    if (xQueueSend(s_key_queue, &event, 0) != pdTRUE)
        ESP_LOGW(TAG, "Key queue full, dropped event");
}

#define KEYS_HELD_MAX 16

typedef struct {
    hid_key_layout_t layout;
    uint8_t held[KEYS_HELD_MAX];
    int held_count;
} keyboard_t;

static void update_held(keyboard_t *kbd, const uint8_t *keys, int count) {
    for (int i = 0; i < kbd->held_count; i++) {
        if (!memchr(keys, kbd->held[i], count))
            post_key(false, kbd->held[i]);
    }
    for (int i = 0; i < count; i++) {
        if (!memchr(kbd->held, keys[i], kbd->held_count))
            post_key(true, keys[i]);
    }
    memcpy(kbd->held, keys, count);
    kbd->held_count = count;
}

static void handle_report(keyboard_t *kbd, const uint8_t *data, size_t length) {
    uint8_t usages[KEYS_HELD_MAX];
    const int n = hid_report_usages(&kbd->layout, data, length, usages, KEYS_HELD_MAX);
    if (n < 0)
        return;
    uint8_t keys[KEYS_HELD_MAX];
    int count = 0;
    for (int i = 0; i < n; i++) {
        const uint8_t key = translate_usage(usages[i]);
        if (key && !memchr(keys, key, count))
            keys[count++] = key;
    }
    update_held(kbd, keys, count);
}

static void interface_callback(hid_host_device_handle_t handle, const hid_host_interface_event_t event,
                               void *arg) {
    keyboard_t *kbd = arg;

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
        uint8_t data[64];
        size_t length = 0;
        if (hid_host_device_get_raw_input_report_data(handle, data, sizeof(data), &length) == ESP_OK)
            handle_report(kbd, data, length);
        break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Keyboard disconnected");
        update_held(kbd, (uint8_t[1]){0}, 0);
        ESP_ERROR_CHECK(hid_host_device_close(handle));
        free(kbd);
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGW(TAG, "Keyboard transfer error");
        break;
    default:
        break;
    }
}

// Opening a device needs control transfers that only the HID driver task can complete,
// so its callback hands new devices to connect_task instead of opening them itself
static void driver_callback(hid_host_device_handle_t handle, const hid_host_driver_event_t event,
                            void *arg) {
    if (event == HID_HOST_DRIVER_EVENT_CONNECTED)
        xQueueSend(s_connect_queue, &handle, 0);
}

static void connect_task(void *arg) {
    hid_host_device_handle_t handle;
    while (true) {
        xQueueReceive(s_connect_queue, &handle, portMAX_DELAY);

        keyboard_t *kbd = calloc(1, sizeof(*kbd));
        if (kbd == NULL)
            continue;
        const hid_host_device_config_t config = {.callback = interface_callback, .callback_arg = kbd};
        if (hid_host_device_open(handle, &config) != ESP_OK) {
            ESP_LOGW(TAG, "Cannot open HID interface");
            free(kbd);
            continue;
        }

        size_t desc_len = 0;
        const uint8_t *desc = hid_host_get_report_descriptor(handle, &desc_len);
        if (desc == NULL || !hid_parse_key_layout(desc, desc_len, &kbd->layout) ||
            hid_host_device_start(handle) != ESP_OK) {
            hid_host_device_close(handle);
            free(kbd);
            continue;
        }
        ESP_LOGI(TAG, "Keyboard connected");
    }
}

static void usb_lib_task(void *arg) {
    while (true) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
    }
}

void input_init(void) {
    s_key_queue = xQueueCreate(32, sizeof(key_event_t));
    s_connect_queue = xQueueCreate(4, sizeof(hid_host_device_handle_t));
    assert(s_key_queue && s_connect_queue);

    const usb_host_config_t host_config = {.intr_flags = ESP_INTR_FLAG_LEVEL1};
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    xTaskCreatePinnedToCore(usb_lib_task, "usb_lib", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(connect_task, "usb_kbd", 4096, NULL, 5, NULL, 1);

    const hid_host_driver_config_t hid_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 1,
        .callback = driver_callback,
    };
    ESP_ERROR_CHECK(hid_host_install(&hid_config));
}

bool input_poll(int *pressed, uint8_t *key) {
    key_event_t event;
    if (xQueueReceive(s_key_queue, &event, 0) != pdTRUE)
        return false;
    *pressed = event.pressed;
    *key = event.key;
    return true;
}
