#include "temperature.h"
#include <math.h>

static accum
temperature_map(uint16_t codepoint, void *map_data)
{
        unsigned accum voltage = adc_as_voltage(codepoint);
        accum volt_diff = voltage - 0.719k;
        accum temp_diff = volt_diff * (1000K / 1.715K);
        accum temp_deg = 25k - temp_diff;
        return temp_deg;
}

struct adc_sensor_data temperature_sensor_data = {
        .channel = ADC_TEMP,
        .map = temperature_map
};
                 
accum
lm19_map(uint16_t codepoint, void *map_data)
{
        float v = 3.3 * codepoint / 0xffff;
        return -1481.96 + sqrt(2.1962e6 + (1.8639 - v) / 3.88e-6);
}
