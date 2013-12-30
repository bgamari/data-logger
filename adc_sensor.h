#pragma once
#include "mchck.h"
#include "sensor.h"

typedef accum (*adc_map_func)(uint16_t voltage, void *map_data);

struct sample_queue {
        struct sensor *sensor;
        struct sample_queue *next;
};

struct adc_sensor_data {
        enum adc_channel channel;
        adc_map_func map;
        void *map_data;

        // internal
        struct sample_queue queue;
};

void adc_sensor_sample(struct sensor *sensor);
