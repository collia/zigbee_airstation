#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "lcd5110_display.h"
#include "lcd5110_font.h"

#define LCD5110_MAX_STRING_LENGTH 14
LOG_MODULE_REGISTER(lcd5110, CONFIG_LCD5110_LOG_LEVEL);

struct lcd5110_data {
    uint8_t max_char_x;
    uint8_t max_char_y;
    uint8_t current_char_x;
    uint8_t current_char_y;
};

const struct device *display_dev;

static struct lcd5110_data lcd5110_data = {
    .max_char_x = 0,
    .max_char_y = 0,
    .current_char_x = 0,
    .current_char_y = 0,
};

static int lcd5110_send_line(char *line, int len);
static int lcd5110_send_ascii_char(int c);

int lcd5110_init(void) {

    struct display_capabilities capabilities;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    int rc = device_init(display_dev);
    if (rc) {
        LOG_ERR("Device %s is not inited, rc = %d\n", display_dev->name, rc);
        return rc;
    }
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device %s not found. Aborting sample.", display_dev->name);
        return 0;
    }

    LOG_INF("Display init for %s", display_dev->name);
    display_get_capabilities(display_dev, &capabilities);
    LOG_INF("Display capabilities: %d x %d", capabilities.x_resolution,
            capabilities.y_resolution);
    lcd5110_data.max_char_x = capabilities.x_resolution / LCD5110_FONT_WIDTH;
    lcd5110_data.max_char_y = capabilities.y_resolution / LCD5110_FONT_HEIGHT;

    lcd5110_printf("LCD initialized");

    return 0;
}
void lcd5110_printf(const char *format, ...) {
    char buffer[LCD5110_MAX_STRING_LENGTH];
    va_list args;

    va_start(args, format);
    int strlen = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    lcd5110_send_line(buffer, strlen);
    return 0;
}

void lcd5110_putc(char c) {
    if (c < LCD5110_FONT_FIRST_CHAR || c > LCD5110_FONT_LAST_CHAR) {
        c = LCD5110_FONT_FIRST_CHAR;
    }
    lcd5110_send_ascii_char(c - LCD5110_FONT_FIRST_CHAR);
}

void lcd5110_setpos(uint32_t x, uint32_t y) {
    if (x >= lcd5110_data.max_char_x) {
        x = lcd5110_data.max_char_x - 1;
    }
    if (y >= lcd5110_data.max_char_y) {
        y = lcd5110_data.max_char_y - 1;
    }
    lcd5110_data.current_char_x = x;
    lcd5110_data.current_char_y = y;
}

static int lcd5110_send_ascii_char(int c) {

    struct display_buffer_descriptor desc = {
        .buf_size = LCD5110_FONT_WIDTH * LCD5110_FONT_HEIGHT / 8,
        .pitch = LCD5110_FONT_WIDTH,
        .width = LCD5110_FONT_WIDTH,
        .height = LCD5110_FONT_HEIGHT,
    };

    int rc = display_write(
        display_dev, lcd5110_data.current_char_x * LCD5110_FONT_WIDTH,
        lcd5110_data.current_char_y * LCD5110_FONT_HEIGHT, &desc, ASCII[c]);
    lcd5110_data.current_char_x++;
    if (lcd5110_data.current_char_x >= lcd5110_data.max_char_x) {
        lcd5110_data.current_char_x = 0;
        lcd5110_data.current_char_y++;
        if (lcd5110_data.current_char_y >= lcd5110_data.max_char_y) {
            lcd5110_data.current_char_y = 0;
        }
    }
    return rc;
}

static int lcd5110_send_line(char *line, int len) {
    static const int char_size = LCD5110_FONT_WIDTH * LCD5110_FONT_HEIGHT / 8;
    uint8_t buffer[LCD5110_FONT_WIDTH * LCD5110_FONT_HEIGHT / 8 *
                   LCD5110_MAX_STRING_LENGTH];
    if (len > LCD5110_MAX_STRING_LENGTH) {
        len = LCD5110_MAX_STRING_LENGTH;
    }
    for (int i = 0; i < len; i++) {
        if (line[i] < LCD5110_FONT_FIRST_CHAR ||
            line[i] > LCD5110_FONT_LAST_CHAR) {
            line[i] = LCD5110_FONT_FIRST_CHAR;
        }
        int c = line[i] - LCD5110_FONT_FIRST_CHAR;
        memcpy(&buffer[char_size * i], ASCII[c], char_size);
    }

    struct display_buffer_descriptor desc = {
        .buf_size = char_size * len,
        .pitch = LCD5110_FONT_WIDTH,
        .width = LCD5110_FONT_WIDTH,
        .height = LCD5110_FONT_HEIGHT,
    };
    int rc = display_write(
        display_dev, lcd5110_data.current_char_x * LCD5110_FONT_WIDTH,
        lcd5110_data.current_char_y * LCD5110_FONT_HEIGHT, &desc, buffer);
    lcd5110_data.current_char_x = 0;
    lcd5110_data.current_char_y++;
    if (lcd5110_data.current_char_y >= lcd5110_data.max_char_y) {
        lcd5110_data.current_char_y = 0;
    }

    return rc;
}
