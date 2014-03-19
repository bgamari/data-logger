#include "sensors/tcs3472.h"
#include "sensor.h"

struct tcs_sensor_data {
        enum tcs_gain gain;
        uint8_t int_time;
        
        // internal
        uint8_t buf;
        struct timeout_ctx timeout;
        struct tcs_ctx tcs;
        struct tcs_sample sample;
        accum values[4];
};

extern struct sensor_type tcs3472_sensor_type;
