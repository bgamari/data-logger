#include "thermistor.h"
#include <math.h>

accum
thermistor_map(uint16_t codepoint, void *map_data)
{
        const struct thermistor_map_data *sd = map_data;
        accum r = 1 / (1 / codepoint - 0xffff) * sd->r1;
        accum temp = 1 / (1 / sd->t0 + 1 / sd->beta * log10(r / sd->r0));
        accum temp_deg = temp + 273;
        return temp_deg;
}

