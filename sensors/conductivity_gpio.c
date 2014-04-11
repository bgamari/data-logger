#include "conductivity_gpio.h"
#include "sensor.h"

struct sensor *_cond_sensor;

void
cond_gpio_init(struct cond_gpio_sensor_data *cond)
{
        pin_mode(cond->pin_a, PIN_MODE_MUX_GPIO);
        gpio_dir(cond->pin_a, GPIO_OUTPUT);
        gpio_write(cond->pin_a, 0);
        SIM.scgc4.cmp = 1;
        int_enable(IRQ_CMP0);
        #ifdef USE_FTM
        SIM.scgc6.ftm1 = 1;
        FTM1.mode.ftmen = 1;
        #endif
}

void
cond_gpio_start(struct sensor *sensor)
{
        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        if (sd->phase) {
                // waiting for rising edge
                CMP0.daccr.vosel = 48;
                CMP0.scr.ief = false;
                CMP0.scr.ier = true;
        } else {
                // waiting for falling edge
                CMP0.daccr.vosel = 16;
                CMP0.scr.ier = false;
                CMP0.scr.ief = true;
        }

        gpio_write(sd->pin_a, sd->phase);

        #ifdef USE_FTM
        FTM1.cnt = 0;
        FTM1.sc.clks = 0x2;
        #endif
}

static void
cond_gpio_sample_done(struct sensor *sensor)
{

        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        #ifdef USE_FTM
        accum dt_ms = 0.03125k * sd->t_accum / (sd->transitions - 1);
        #else
        uint32_t dt = timeout_get_time().time - sd->start_time.time;
        accum dt_ms = 1.0k * dt / sd->transitions;
        timeout_put_ref();
        #endif

        CMP0.cr1.en = 0;
        CMP0.daccr.dacen = 0;
        gpio_write(sd->pin_a, 0);

        sensor_new_sample(sensor, &dt_ms);
}

void
CMP0_Handler(void)
{
        struct cond_gpio_sensor_data *sd = _cond_sensor->sensor_data;

        #ifdef USE_FTM
        FTM1.sc.clks = 0x0;

        /*
         * we need to throw out the first transition as it is measuring
         * the rise time from ground
         */
        if (sd->count != sd->transitions && !sd->phase)
                sd->t_accum += FTM1.cnt;
        #endif

        CMP0.scr.raw |= ((struct CMP_SCR_t) {.cfr = 1, .cff = 1}).raw;
        if (!sd->phase)
                sd->count--;
        sd->phase = !sd->phase;
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
        sd->count = sd->transitions;
        sd->phase = true;
        sd->t_accum = 0;

        #ifdef USE_FTM
        FTM1.cntin = 0;
        FTM1.mod = 0xffff;
        #else
        timeout_get_ref();
        sd->start_time = timeout_get_time();
        #endif

        pin_mode(PIN_PTC6, PIN_MODE_MUX_ANALOG);
        CMP0.daccr.vrsel = 1; // Vin2 == Vcc
        CMP0.daccr.dacen = 1;
        CMP0.muxcr.psel = 0;
        CMP0.muxcr.msel = 7;
        CMP0.fpr = 10;
        CMP0.cr0.filter_cnt = 7;
        CMP0.cr0.hystctr = 3;
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
