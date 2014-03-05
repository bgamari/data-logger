#include "sensors/battery_voltage.h"
#include "sensors/adc_sensor.h"

struct measurable batt_v_measurables[] = {
        {.id = 0, .name = "battery voltage", .unit = "Volts"}
};

struct sensor_type batt_v_sensor_type = {
        .sample_fn = &batt_v_sample,
        /* We are using the bus clock therefore we must not enter STOP */
        .no_stop = true,
        .n_measurables = 1,
        .measurables = batt_v_measurables
};

static void
batt_v_sample_done(uint16_t codepoint, int error, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct batt_v_sensor_data *sd = sensor->sensor_data;
        gpio_write(sd->enable_pin, GPIO_HIGH);
        accum batt_v = adc_as_voltage(codepoint) * sd->divider;
        sensor_new_sample(sensor, 0, batt_v);
}

void
batt_v_sample(struct sensor *sensor)
{
        struct batt_v_sensor_data *sd = sensor->sensor_data;
        adc_sensor_init();
        gpio_dir(sd->enable_pin, GPIO_OUTPUT);
        gpio_write(sd->enable_pin, GPIO_LOW);
        adc_queue_sample(&sd->ctx, sd->channel, ADC_MODE_AVG_32,
                         batt_v_sample_done, sensor);
}

void
batt_v_init(struct sensor *sensor)
{
        struct batt_v_sensor_data *sd = sensor->sensor_data;
        enum gpio_pin_id pin = sd->enable_pin;
        gpio_dir(pin, GPIO_DISABLE);
        pin_mode(pin, PIN_MODE_MUX_GPIO | PIN_MODE_PULLUP);
}
