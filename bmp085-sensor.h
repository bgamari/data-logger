#pragma once
#include "bmp085.h"
#include "sensor.h"

struct bmp085_sensor_data {
        struct bmp085_ctx ctx;
        uint16_t last_temperature, last_pressure;
};

extern struct sensor_type bmp085_sensor_type;
