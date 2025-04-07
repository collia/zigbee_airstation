#pragma once
#include <zephyr/drivers/sensor.h>

struct bme280_result {
    struct sensor_value temperature;
    struct sensor_value pressure;
    struct sensor_value humidity;
};

const struct device *bme280_sensor_init(void);
int bme280_sensor_read(const struct device *dev, const struct bme280_result* result);
