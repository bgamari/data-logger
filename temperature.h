#pragma once
#include "adc_sensor.h"

extern struct adc_sensor_data temperature_sensor_data;

accum lm19_map(uint16_t codepoint, void *map_data);
