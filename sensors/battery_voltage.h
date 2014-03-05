#include "sensor.h"

struct batt_v_sensor_data {
        enum adc_channel channel;
        enum gpio_pin_id enable_pin;
        unsigned accum divider;

        // internal
        struct adc_queue_ctx ctx;
};

extern struct sensor_type batt_v_sensor_type;

void batt_v_sample(struct sensor *sensor);

void batt_v_init(struct sensor *sensor);
