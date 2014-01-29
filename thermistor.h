#pragma once
#include "adc_sensor.h"

enum thermistor_polarity {
        // thermistor on the high side, fixed resistor on low
        THERMISTOR_HIGH,
        // thermistor on the low side, fixed resistor on high
        THERMISTOR_LOW
};

struct thermistor_map_data {
        accum beta;
        accum t0;
        accum r0;
        accum r1;
        enum thermistor_polarity polarity;
};
        
accum
thermistor_map(uint16_t codepoint, void *map_data);

