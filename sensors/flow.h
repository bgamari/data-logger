#pragma once
#include <mchck.h>
#include "sensor.h"

struct flow_sensor_data {
        unsigned int count;

        // private
        uint32_t tick_accum;
        unsigned int tick_count;
        uint32_t last_tick;
};

extern struct sensor_type flow_sensor_type;
