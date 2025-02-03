/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <zephyr/kernel.h>

#include "ccs811_sensor.h"
/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define POWER_PIN_EN_NODE DT_NODELABEL(powerpin0)

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

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);

        // fetch_sensor(dev);
    }
    return 0;
}
