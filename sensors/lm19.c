#include <mchck.h>
#include <math.h>
#include "sensors/lm19.h"

accum
lm19_map(uint16_t codepoint, void *map_data)
{
        accum v = adc_as_voltage(codepoint);
        // linear approximation for -40oC to +85oC
        return (v - 1.8583) * 1000 / -11.67;
}
