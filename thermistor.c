#include "thermistor.h"
#include <math.h>

accum
thermistor_map(uint16_t codepoint, void *map_data)
{
        const struct thermistor_map_data *sd = map_data;
        accum a = 1k * 0xffff / codepoint;
        accum r;
        if (sd->polarity == THERMISTOR_HIGH) {
                r = sd->r1 / (a - 1);
        } else {
                r = sd->r1 * (1 - a) / a;
        }
        accum temp = 1 / (1 / sd->t0 + 1 / sd->beta * log10(r / sd->r0));
        return temp;
}

