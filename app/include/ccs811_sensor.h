#pragma once

#include <zephyr/drivers/sensor.h>

struct ccs811_result {
    struct sensor_value co2;
    struct sensor_value tvoc;
};

typedef void (*cc811_sensor_cb)(struct ccs811_result*);
int ccs811_sensor_init(cc811_sensor_cb cb);
