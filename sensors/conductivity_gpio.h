#include <mchck.h>

struct cond_gpio_sensor_data {
        enum gpio_pin_id pin_a, pin_b;
        unsigned int transitions;

        // private
        bool phase;
        unsigned int count;
        union timeout_time start_time;
};

struct sensor_type cond_gpio_sensor_type;

void cond_gpio_init(struct cond_gpio_sensor_data *cond);
