#include "conductivity_gpio.h"
#include "sensor.h"

unsigned int count = 0;

struct sensor *_cond_sensor;

void
cond_gpio_init(struct cond_gpio_sensor_data *cond)
{
        pin_mode(cond->pin_a, PIN_MODE_MUX_GPIO);
        gpio_dir(cond->pin_a, GPIO_OUTPUT);
        gpio_write(cond->pin_a, 0);
        SIM.scgc4.cmp = 1;
        int_enable(IRQ_CMP0);
}

void
cond_gpio_start(struct sensor *sensor)
{
        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        if (sd->phase) {
                // waiting for rising edge
                CMP0.scr.ief = false;
                CMP0.scr.ier = true;
        } else {
                // waiting for falling edge
                CMP0.scr.ier = false;
                CMP0.scr.ief = true;
        }

        CMP0.daccr.vosel = (sd->phase + 1) * 64 / 3;
        gpio_write(sd->pin_a, sd->phase);
}

static void
cond_gpio_sample_done(struct sensor *sensor)
{

        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        uint32_t dt = timeout_get_time().time - sd->start_time.time;
        accum dt_accum = 1.0 * dt / sd->transitions;

        CMP0.cr1.en = 0;
        CMP0.daccr.dacen = 0;
        timeout_put_ref();
        gpio_write(sd->pin_a, 0);

        sensor_new_sample(sensor, &dt_accum);
}

void
CMP0_Handler(void)
{
        struct cond_gpio_sensor_data *sd = _cond_sensor->sensor_data;
        CMP0.scr.raw |= ((struct CMP_SCR_t) {.cfr = 1, .cff = 1}).raw;
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
        sd->phase = true;

        pin_mode(PIN_PTC6, PIN_MODE_MUX_ANALOG);
        CMP0.daccr.vrsel = 1; // Vin2 == Vcc
        CMP0.daccr.dacen = 1;
        CMP0.muxcr.psel = 0;
        CMP0.muxcr.msel = 7;
        CMP0.fpr = 10;
        CMP0.cr0.filter_cnt = 6;
        CMP0.cr1.en = 1;
        CMP0.scr.ier = 1;

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
