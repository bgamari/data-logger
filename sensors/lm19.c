#include <math.h>

accum
lm19_map(uint16_t codepoint, void *map_data)
{
        float v = 3.3 * codepoint / 0xffff;
        return -1481.96 + sqrt(2.1962e6 + (1.8639 - v) / 3.88e-6);
}
