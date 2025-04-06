/*
 * Copyright (c) Nikolay Klimchuk
 */

#include <stdint.h>
#include <stdio.h>
#define DT_DRV_COMPAT zephyr_lcd5110
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lcd5110_drv, CONFIG_LCD5110_LOG_LEVEL);

struct lcd5110_config {
    const struct device *mipi_dbi;
    const struct mipi_dbi_config dbi_config;
    uint16_t height;
    uint16_t width;
};

struct lcd5110_data {
    enum display_pixel_format current_pixel_format;
};

/* --- L C D   S T A R T S   H E R E --------------------------------------- */
/**
 * @brief Send a command to the LCD5110 display.
 *
 * @param config Pointer to the LCD5110 configuration structure.
 * @param cmd Command to send to the display.
 * @return 0 on success, negative error code on failure.
 */
static int lcd5110_send_cmd(const struct lcd5110_config *config, uint8_t cmd) {
    LOG_INF("lcd5110_send_cmd %x", cmd);

    int ret = mipi_dbi_command_write(config->mipi_dbi, &config->dbi_config, cmd,
                                     NULL, 0);

    if (ret != 0) {
        LOG_ERR("Failed to send command: 0x%02x, error: %d", cmd, ret);
    }
    return ret;
}

/**
 * @brief Send a framebuffer to the LCD5110 display.
 *
 * @param config Pointer to the LCD5110 configuration structure.
 * @param framebuffer Pointer to the framebuffer data.
 * @param size Size of the framebuffer data.
 * @return 0 on success, negative error code on failure.
 */
static int lcd5110_send_framebuffer(const struct lcd5110_config *config,
                                    const uint8_t *framebuffer, size_t size) {
    struct display_buffer_descriptor desc = {
        .buf_size = size,
        .pitch = config->width,
        .width = config->width,
        .height = config->height,
    };
    return mipi_dbi_write_display(config->mipi_dbi, &config->dbi_config,
                                  framebuffer, &desc, PIXEL_FORMAT_MONO01);
}

/**
 * @brief Clear the LCD5110 display screen.
 *
 * @param config Pointer to the LCD5110 configuration structure.
 */
static void clearLcdScreen(const struct lcd5110_config *config) {
    uint8_t framebuffer[84 * 48 / 8];
    memset(framebuffer, 0x0, sizeof(framebuffer));
    lcd5110_send_framebuffer(config, framebuffer, sizeof(framebuffer));
    lcd5110_send_cmd(config, 0x80 | 0); // set x coordinate to 0
    lcd5110_send_cmd(config, 0x40 | 0); // set y coordinate to 0
}

/**
 * @brief Initialize the LCD5110 display.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, negative error code on failure.
 */
static int lcd5110_init(const struct device *dev) {
    struct lcd5110_data *disp_data = dev->data;
    const struct lcd5110_config *config = dev->config;

    LOG_INF("Init display %s, %x", dev->name, disp_data);
    disp_data->current_pixel_format = PIXEL_FORMAT_MONO01;

    int ret = mipi_dbi_reset(config->mipi_dbi, 100);
    if (ret < 0) {
        LOG_ERR("mipi_dbi_reset fail %s, rc=%d", dev->name, ret);
        return ret;
    }
    k_msleep(200);

    // init LCD
    lcd5110_send_cmd(config, 0x21); // LCD Extended Commands
    lcd5110_send_cmd(config, 0xc0); // Set LCD Cop (Contrast).	//0xb1
    lcd5110_send_cmd(config, 0x04); // Set Temp coefficent.		//0x04
    lcd5110_send_cmd(config, 0x13); // Set contrast 3
    lcd5110_send_cmd(config, 0x0c); // LCD in normal mode. 0x0d inverse mode
    lcd5110_send_cmd(config, 0x20);
    lcd5110_send_cmd(config, 0x0c);
    clearLcdScreen(config);

    return 0;
}

static int lcd5110_write(const struct device *dev, const uint16_t x,
                         const uint16_t y,
                         const struct display_buffer_descriptor *desc,
                         const void *buf) {
    const struct lcd5110_config *config = dev->config;

    __ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
    __ASSERT(desc->pitch <= config->width,
             "Pitch in descriptor is larger than screen size");
    __ASSERT(desc->height <= config->height,
             "Height in descriptor is larger than screen size");
    __ASSERT(x + desc->pitch <= config->width,
             "Writing outside screen boundaries in horizontal direction");
    __ASSERT(y + desc->height <= config->height,
             "Writing outside screen boundaries in vertical direction");

    if (desc->width > desc->pitch || x + desc->pitch > config->width ||
        y + desc->height > config->height) {
        return -EINVAL;
    }
    __ASSERT(config->mipi_dbi, "mipi_dbi is empty");
    mipi_dbi_write_display(config->mipi_dbi, &config->dbi_config, buf, desc,
                           PIXEL_FORMAT_MONO01);
    return 0;
}

static int dummy_display_blanking_off(const struct device *dev) { return 0; }

static int dummy_display_blanking_on(const struct device *dev) { return 0; }

static int dummy_display_set_brightness(const struct device *dev,
                                        const uint8_t brightness) {
    return 0;
}

static int lcd5110_display_set_contrast(const struct device *dev,
                                        const uint8_t contrast) {
    const struct lcd5110_config *config = dev->config;
    lcd5110_send_cmd(config, 0x21); // LCD Extended Commands
    lcd5110_send_cmd(config, 0x10 | (contrast & 0x7));
    lcd5110_send_cmd(config, 0x20);

    return 0;
}

static void
lcd5110_display_get_capabilities(const struct device *dev,
                                 struct display_capabilities *capabilities) {
    const struct lcd5110_config *config = dev->config;
    struct lcd5110_data *disp_data = dev->data;

    memset(capabilities, 0, sizeof(struct display_capabilities));
    capabilities->x_resolution = config->width;
    capabilities->y_resolution = config->height;
    capabilities->supported_pixel_formats = PIXEL_FORMAT_MONO01;
    capabilities->current_pixel_format = disp_data->current_pixel_format;
    capabilities->screen_info =
        SCREEN_INFO_MONO_VTILED | SCREEN_INFO_MONO_MSB_FIRST;
}

static int
lcd5110_display_set_pixel_format(const struct device *dev,
                                 const enum display_pixel_format pixel_format) {
    struct lcd5110_data *disp_data = dev->data;

    disp_data->current_pixel_format = pixel_format;
    return 0;
}

static const struct display_driver_api lcd5110_api = {
    .blanking_on = dummy_display_blanking_on,
    .blanking_off = dummy_display_blanking_off,
    .write = lcd5110_write,
    .set_brightness = dummy_display_set_brightness,
    .set_contrast = lcd5110_display_set_contrast,
    .get_capabilities = lcd5110_display_get_capabilities,
    .set_pixel_format = lcd5110_display_set_pixel_format,
};

#define LCD5110_DEFINE(n)                                                      \
    static const struct lcd5110_config lcd5110_config_##n = {                  \
        .height = DT_INST_PROP(n, height),                                     \
        .width = DT_INST_PROP(n, width),                                       \
        .mipi_dbi = DEVICE_DT_GET(DT_INST_PARENT(n)),                          \
        .dbi_config =                                                          \
            {                                                                  \
                .config = MIPI_DBI_SPI_CONFIG_DT(                              \
                    DT_DRV_INST(n), SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),  \
                .mode =                                                        \
                    DT_INST_PROP_OR(n, mipi_mode, MIPI_DBI_MODE_SPI_4WIRE),    \
            },                                                                 \
    };                                                                         \
                                                                               \
    static struct lcd5110_data lcd5110_data_##n;                               \
                                                                               \
    DEVICE_DT_INST_DEFINE(n, &lcd5110_init, NULL, &lcd5110_data_##n,           \
                          &lcd5110_config_##n, POST_KERNEL,                    \
                          CONFIG_DISPLAY_INIT_PRIORITY, &lcd5110_api);

DT_INST_FOREACH_STATUS_OKAY(LCD5110_DEFINE)
