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
/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define POWER_PIN_EN_NODE DT_NODELABEL(powerpin0)

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

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
    ccs811_sensor_init();
    int ret;
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    int i = 0;
    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        // printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);

        if (i > 20) {
            main_display_test();
            i = 0;
        }
    }
    return 0;
}

void read_bme280(void) {
    k_msleep(2000);
    const struct device *bme280_dev = bme280_sensor_init();
    while (bme280_dev) {
        bme280_sensor_read(bme280_dev);
        k_msleep(1000);
    }
}

void display_test(void) {
    // main_display_test();
    lcd5110_init();
    k_msleep(1000);
    char c = ' ';
    char buf[2] = {c, 0};
    while (1) {
        buf[0] = c++;
        lcd5110_write(buf);
        k_msleep(1000);
        if (c > 0x7f) {
            c = ' ';
        }
    }
}

K_THREAD_DEFINE(read_bme280_id, STACKSIZE, read_bme280, NULL, NULL, NULL,
                PRIORITY, 0, 0);
K_THREAD_DEFINE(display_test_id, STACKSIZE, display_test, NULL, NULL, NULL,
                PRIORITY, 0, 0);
// K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
//                 PRIORITY, 0, 0);
