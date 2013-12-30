#pragma once
#include "adc_sensor.h"

struct thermistor_map_data {
        accum beta;
        accum t0;
        accum r0;
        accum r1;
};
        
accum
thermistor_map(uint16_t codepoint, void *map_data);

