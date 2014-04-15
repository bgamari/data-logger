#include <mchck.h>

#define USE_VREF

struct cond_gpio_sensor_data {
        enum gpio_pin_id pin_a;
        unsigned int transitions;

        // private
        unsigned int t_accum[2];
        bool phase;
        unsigned int count;
};

struct sensor_type cond_gpio_sensor_type;

void cond_gpio_init(struct cond_gpio_sensor_data *cond);
