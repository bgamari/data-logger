#include <mchck.h>

#define USE_FTM
#define USE_VREF

struct cond_gpio_sensor_data {
        enum gpio_pin_id pin_a;
        unsigned int transitions;

        // private
        #ifdef USE_FTM
        unsigned int t_accum;
        #endif
        bool phase;
        unsigned int count;
        union timeout_time start_time;
};

struct sensor_type cond_gpio_sensor_type;

void cond_gpio_init(struct cond_gpio_sensor_data *cond);
