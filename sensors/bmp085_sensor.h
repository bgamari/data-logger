#pragma once
#include "sensor.h"
#include "sensors/bmp085.h"

struct bmp085_sensor_data {
        struct bmp085_ctx ctx;
        uint16_t last_temperature, last_pressure;
        accum values[2];
};

extern struct sensor_type bmp085_sensor_type;
