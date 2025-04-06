#pragma once
const struct device *bme280_sensor_init(void);
void bme280_sensor_read(const struct device *dev);