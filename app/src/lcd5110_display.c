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

LOG_MODULE_REGISTER(lcd5110, CONFIG_LCD5110_LOG_LEVEL);

/* --- L C D   S P E C I F I C --------------------------------------------- */

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

const struct device *display_dev;

static void writeCharToLcd(char);
static void writeStringToLcd(char *);

int lcd5110_init(void) {

    struct display_capabilities capabilities;
    size_t x;
    size_t y;

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
    writeStringToLcd("LCD initialized");

    return 0;
}

size_t lcd5110_write(char *buf) {
    writeStringToLcd(buf);
    return 0;
}

static int lcd5110_send_ascii_char(int c) {
    if (c < 0 || c >= 96) {
        return -EINVAL; // Return an error if the index is out of bounds
    }
    struct display_buffer_descriptor desc = {
        .buf_size = LCD5110_FONT_WIDTH,
        .pitch = 5,
        .width = 5,
        .height = LCD5110_FONT_HEIGHT,
    };
    return display_write(display_dev, 0, 0, &desc, ASCII[c]);
}

static void writeCharToLcd(char data) {
    if (data < LCD5110_FONT_FIRST_CHAR || data > LCD5110_FONT_LAST_CHAR) {
        lcd5110_send_ascii_char(LCD5110_FONT_FIRST_CHAR);
    } else {
        lcd5110_send_ascii_char(data - LCD5110_FONT_FIRST_CHAR);
    }
}

static void writeStringToLcd(char *data) {
    while (*data)
        writeCharToLcd(*data++);
}
