#include "sensors/tcs3472.h"
#include "sensor.h"

struct tcs_sensor_data {
        struct tcs_ctx ctx;
        struct tcs_sample sample;
};

extern struct sensor_type tcs3472_sensor_type;
