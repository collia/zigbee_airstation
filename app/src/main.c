/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <zephyr/kernel.h>

#include "bme280_sensor.h"
#include "ccs811_sensor.h"
// #include "st7789v_display.h"
#include "lcd5110_display.h"

#define SLEEP_TIME_MS 500

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define POWER_PIN_EN_NODE DT_NODELABEL(powerpin0)

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

enum sensor_type { CCS811_SENSOR, BME280_SENSOR };

struct sensor_data {
    enum sensor_type type;
    union {
        struct ccs811_result ccs811;
        struct bme280_result bme280;
    } data;
};

static void display_handler(void);
static void read_ccs811(struct ccs811_result *);
static void read_bme280(void);
static int enable_power_line(void);

static int enable_power_line() {
    int ret;
    static const struct gpio_dt_spec power_pin_en =
        GPIO_DT_SPEC_GET(POWER_PIN_EN_NODE, gpios);
    if (!gpio_is_ready_dt(&power_pin_en)) {
        printk("Error: Source Enable device %s is not ready",
               power_pin_en.port->name);
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&power_pin_en, GPIO_OUTPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure Source Enable device %s pin %d",
               ret, power_pin_en.port->name, power_pin_en.pin);
        return ret;
    }
    ret = gpio_pin_set_dt(&power_pin_en, 0);
    if (ret != 0) {
        printk("Error %d: failed to enable source", ret);
        return ret;
    }
    printk("Device %s, port %d set to 0\n", power_pin_en.port->name,
           power_pin_en.pin);
    return 0;
}

int main(void) {
    enable_power_line();
    k_msleep(100);
    ccs811_sensor_init(read_ccs811);
    int ret;
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        // printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}

K_FIFO_DEFINE(display_fifo);

static void read_ccs811(struct ccs811_result *data) {
    size_t size = sizeof(struct sensor_data);
    struct sensor_data *mem_ptr = (struct sensor_data *)k_malloc(size);
    __ASSERT_NO_MSG(mem_ptr != 0);
    mem_ptr->type = CCS811_SENSOR;
    memcpy(&mem_ptr->data.ccs811, data, size);

    k_fifo_put(&display_fifo, mem_ptr);
}

static void read_bme280(void) {
    k_msleep(2000);
    const struct device *bme280_dev = bme280_sensor_init();
    const struct bme280_result data;
    const size_t size = sizeof(struct sensor_data);

    while (bme280_dev) {
        int rc = bme280_sensor_read(bme280_dev, &data);
        if (rc == 0) {
            struct sensor_data *mem_ptr = (struct sensor_data *)k_malloc(size);
            __ASSERT_NO_MSG(mem_ptr != 0);
            mem_ptr->type = BME280_SENSOR;
            memcpy(&mem_ptr->data.bme280, &data, size);

            k_fifo_put(&display_fifo, mem_ptr);
        }
        k_msleep(1000);
    }
}

static void display_handler(void) {
    // main_display_test();
    lcd5110_init();
    k_msleep(1000);
    while (1) {
        struct sensor_data *rx_data = k_fifo_get(&display_fifo, K_FOREVER);
        if (rx_data == NULL) {
            continue;
        }
        if (rx_data->type == CCS811_SENSOR) {
            printf("CCS811: %u ppm eCO2; %u ppb eTVOC\n",
                   rx_data->data.ccs811.co2.val1,
                   rx_data->data.ccs811.tvoc.val1);
            lcd5110_setpos(0, 0);
            lcd5110_printf("CO2 %u ppm    ", rx_data->data.ccs811.co2.val1);
            lcd5110_setpos(0, 1);
            lcd5110_printf("TVOC %u ppb    ", rx_data->data.ccs811.tvoc.val1);
        } else if (rx_data->type == BME280_SENSOR) {
            printf("BME280: %u %u %u", rx_data->data.bme280.temperature.val1);
            lcd5110_setpos(0, 2);
            lcd5110_printf("%u C, %u %%  ",
                           rx_data->data.bme280.temperature.val1,
                           rx_data->data.bme280.humidity.val1);
            lcd5110_printf("%u.%06u kPa  ", rx_data->data.bme280.pressure.val1,
                           rx_data->data.bme280.pressure.val2);
        }
        k_free(rx_data);
    }
}

K_THREAD_DEFINE(read_bme280_id, STACKSIZE, read_bme280, NULL, NULL, NULL,
                PRIORITY, 0, 0);
K_THREAD_DEFINE(display_handler_id, STACKSIZE, display_handler, NULL, NULL,
                NULL, PRIORITY, 0, 0);
