#include "conductivity_gpio.h"
#include "sensor.h"

unsigned int count = 0;

struct sensor *_cond_sensor;

void
cond_gpio_init(struct cond_gpio_sensor_data *cond)
{
        pin_mode(cond->pin_a, PIN_MODE_MUX_GPIO);
        pin_mode(cond->pin_b, PIN_MODE_MUX_GPIO);
        gpio_dir(cond->pin_a, GPIO_OUTPUT);
        gpio_dir(cond->pin_b, GPIO_OUTPUT);
        gpio_write(cond->pin_a, 0);
        gpio_write(cond->pin_b, 0);

        pin_mode(PIN_PTC2, PIN_MODE_MUX_ANALOG);
        SIM.scgc4.cmp = 1;
}

void
cond_gpio_start(struct sensor *sensor)
{
        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        gpio_write(sd->pin_a, sd->phase);
        gpio_write(sd->pin_b, !sd->phase);
        CMP1.cr1.inv = sd->phase;
}

static void
cond_gpio_sample_done(struct sensor *sensor)
{

        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        uint32_t dt = timeout_get_time().time - sd->start_time.time;
        accum dt_accum = dt / 1000;

        CMP1.cr1.en = 0;
        CMP1.daccr.dacen = 0;
        timeout_put_ref();
        gpio_dir(sd->pin_a, GPIO_DISABLE);
        gpio_dir(sd->pin_b, GPIO_DISABLE);

        sensor_new_sample(sensor, &dt_accum);
}

void
CMP_Handler(void)
{
        struct cond_gpio_sensor_data *sd = _cond_sensor->sensor_data;
        CMP1.scr.cfr = 1;
        sd->phase = !sd->phase;
        sd->count--;
        if (sd->count > 0)
                cond_gpio_start(_cond_sensor);
        else
                cond_gpio_sample_done(_cond_sensor);
}

static void
cond_gpio_sample(struct sensor *sensor)
{
        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        _cond_sensor = sensor;
        timeout_get_ref();
        sd->start_time = timeout_get_time();
        sd->count = sd->transitions;

        CMP1.daccr.dacen = 1;
        CMP1.daccr.vrsel = 1;
        CMP1.daccr.vosel = 64/2;
        CMP1.cr1.pmode = 0;
        CMP1.muxcr.psel = 0x0;
        CMP1.muxcr.msel = 0x7;
        CMP1.cr1.en = 1;
        CMP1.scr.ier = 1;

        cond_gpio_start(sensor);
}

static struct measurable cond_gpio_measurables[] = {
        {.id = 0, .name = "conductivity", .unit = "arbitrary"},
};

struct sensor_type cond_gpio_sensor_type = {
        .sample_fn = &cond_gpio_sample,
        .no_stop = true,
        .n_measurables = 1,
        .measurables = cond_gpio_measurables
};
