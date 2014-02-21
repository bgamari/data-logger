#pragma once
#include "sensors/adc_sensor.h"

extern struct adc_sensor_data core_temperature_sensor_data;
extern struct sensor_type core_temp_sensor_type;

accum lm19_map(uint16_t codepoint, void *map_data);
