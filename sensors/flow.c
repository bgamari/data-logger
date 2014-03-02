#include "flow.h"

#define FLOW_PIN PIN_PTB17

static struct sensor *_sensor;

static void
flow_set_enabled(bool enabled)
{
        enum pin_mode mode = enabled ? PIN_MODE_PULLUP : PIN_MODE_PULLDOWN;
        pin_mode(FLOW_PIN, mode | PIN_MODE_FILTER_ON | PIN_MODE_MUX_GPIO);
}

static void
flow_on_change(void *cbdata)
{
        struct flow_sensor_data *sd = _sensor->sensor_data;
        uint16_t t = timeout_get_time().time;
        if (sd->last_tick == 0) {
                // start of sample
                sd->last_tick = t;
        } else if (t < sd->last_tick) {
                // overflow
                sd->last_tick = t;
        } else if (t < 10 + sd->last_tick) {
                // debounce
        } else {
                uint32_t dt = t - sd->last_tick;
                sd->tick_accum += dt;
                sd->tick_count--;
                sd->last_tick = t;
                if (sd->tick_count == 0) {
                        flow_set_enabled(false);
                        timeout_put_ref();
                        sensor_new_sample(_sensor, 0, 1. * sd->tick_accum / sd->count);
                }
        }
                
}
PIN_DEFINE_CALLBACK(FLOW_PIN, PIN_CHANGE_RISING, flow_on_change, NULL);

void
flow_sample(struct sensor *sensor)
{
        _sensor = sensor;
        struct flow_sensor_data *sd = _sensor->sensor_data;
        sd->tick_accum = 0;
        sd->tick_count = sd->count;
        sd->last_tick = 0;
        timeout_get_ref();
        flow_set_enabled(true);
}

static struct measurable flow_measurables[] = {
        {.id = 0, .name = "frequency", .unit = "milliseconds"}
};

struct sensor_type flow_sensor_type = {
        .sample_fn = flow_sample,
        .no_stop = false,
        .n_measurables = 1,
        .measurables = flow_measurables
};
