#include <bme280_sensor.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/dsp/print_format.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme280);

SENSOR_DT_READ_IODEV(iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme280),
                     {SENSOR_CHAN_AMBIENT_TEMP, 0}, {SENSOR_CHAN_HUMIDITY, 0},
                     {SENSOR_CHAN_PRESS, 0});

RTIO_DEFINE(ctx, 1, 1);

const struct device *bme280_sensor_init(void) {
    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        printk("\nError: no device found.\n");
        return NULL;
    }
    int rc = device_init(dev);
    if (rc) {
        printk("Device %s is not inited, rc = %d\n", dev->name, rc);
        return NULL;
    }
    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready; "
               "check the driver initialization logs for errors.\n",
               dev->name);
        return NULL;
    }

    printk("Found device \"%s\", getting sensor data\n", dev->name);
    return dev;
}

int bme280_sensor_read(const struct device *dev,
                       const struct bme280_result *result) {
    // struct sensor_value temp, press, humidity;
    int rc;
    if (dev == NULL) {
        printk("Device is NULL\n");
        return -1;
    }

    rc = sensor_sample_fetch(dev);
    if (rc != 0) {
        printk("Failed to fetch sensor data: %d\n", rc);
        return rc;
    }

    rc =
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &result->temperature);
    if (rc == 0) {
        printk("Temperature: %d.%06d C\n", result->temperature.val1,
               result->temperature.val2);
    } else {
        printk("Failed to get temperature data: %d\n", rc);
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &result->pressure);
    if (rc == 0) {
        printk("Pressure: %d.%06d kPa\n", result->pressure.val1,
               result->pressure.val2);
    } else {
        printk("Failed to get pressure data: %d\n", rc);
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &result->humidity);
    if (rc == 0) {
        printk("Humidity: %d.%06d %%\n", result->humidity.val1,
               result->humidity.val2);
    } else {
        printk("Failed to get humidity data: %d\n", rc);
        return rc;
    }
    return 0;
}
