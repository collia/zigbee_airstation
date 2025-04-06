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

void bme280_sensor_read(const struct device *dev) {
    struct sensor_value temp, press, humidity;
    int rc;
    if (dev == NULL) {
        printk("Device is NULL\n");
        return;
    }

    rc = sensor_sample_fetch(dev);
    if (rc != 0) {
        printk("Failed to fetch sensor data: %d\n", rc);
        return;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (rc == 0) {
        printk("Temperature: %d.%06d C\n", temp.val1, temp.val2);
    } else {
        printk("Failed to get temperature data: %d\n", rc);
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
    if (rc == 0) {
        printk("Pressure: %d.%06d kPa\n", press.val1, press.val2);
    } else {
        printk("Failed to get pressure data: %d\n", rc);
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
    if (rc == 0) {
        printk("Humidity: %d.%06d %%\n", humidity.val1, humidity.val2);
    } else {
        printk("Failed to get humidity data: %d\n", rc);
    }
}

/*
void bme280_sensor_read(const struct device *dev) {
    uint8_t buf[256] = {0};

    int rc = sensor_read(&iodev, &ctx, buf, sizeof(buf));

    if (rc != 0) {
        printk("%s: sensor_read() failed: %d\n", dev->name, rc);
        return rc;
    }
     // for (size_t i = 0; i < sizeof(buf); i++) {
     //     if (i % 16 == 0) {
     //         printf("\n");
     //     }
     //     printf(" %02hhx", buf[i]);
     // }
    const struct sensor_decoder_api *decoder;

    rc = sensor_get_decoder(dev, &decoder);

    if (rc != 0) {
        printk("%s: sensor_get_decode() failed: %d\n", dev->name, rc);
        return rc;
    }

    uint32_t temp_fit = 0;
    struct sensor_q31_data temp_data = {0};

    decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0},
                    &temp_fit, 1, &temp_data);

    uint32_t press_fit = 0;
    struct sensor_q31_data press_data = {0};

    decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_PRESS, 0},
                    &press_fit, 1, &press_data);

    uint32_t hum_fit = 0;
    struct sensor_q31_data hum_data = {0};

    decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_HUMIDITY, 0},
                    &hum_fit, 1, &hum_data);

    printk("temp: %s%d.%d; press: %s%d.%d; humidity: %s%d.%d\n",
           PRIq_arg(temp_data.readings[0].temperature, 6, temp_data.shift),
           PRIq_arg(press_data.readings[0].pressure, 6, press_data.shift),
           PRIq_arg(hum_data.readings[0].humidity, 6, hum_data.shift));
}

*/