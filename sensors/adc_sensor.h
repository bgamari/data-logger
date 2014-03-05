#pragma once
#include "mchck.h"
#include "sensor.h"

typedef accum (*adc_map_func)(uint16_t codepoint, void *map_data);

struct adc_sensor_data {
        enum adc_channel channel;
        enum adc_mode mode;
        adc_map_func map;
        void *map_data;

        // internal
        struct adc_queue_ctx ctx;
};

extern struct sensor_type adc_sensor;

void adc_sensor_sample(struct sensor *sensor);

// A mapping function returning the codepoint
accum adc_map_raw(uint16_t codepoint, void *map_data);

// helper to ensure ADC has been initialized
void adc_sensor_init();

// poked at in power.c to ensure ADC is reinitialized as necessary
// after leaving low-power modes
extern bool adc_initialized;
