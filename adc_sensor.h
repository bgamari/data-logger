#pragma once
#include "mchck.h"
#include "sensor.h"

typedef accum (*adc_map_func)(uint16_t voltage, void *map_data);

struct adc_sensor_data {
        enum adc_channel channel;
        adc_map_func map;
        void *map_data;
};

void adc_sensor_sample(struct sensor *sensor);
