#include "conductivity_gpio.h"
#include "sensor.h"

struct sensor *_cond_sensor;

void
cond_gpio_init(struct cond_gpio_sensor_data *cond)
{
        pin_mode(cond->pin_a, PIN_MODE_MUX_GPIO | PIN_MODE_SLEW_FAST);
        gpio_dir(cond->pin_a, GPIO_OUTPUT);
        gpio_write(cond->pin_a, 0);
        SIM.scgc4.cmp = 1;
        int_enable(IRQ_CMP0);
        SIM.scgc6.ftm1 = 1;
        FTM1.mode.ftmen = 1;
}

void
cond_gpio_start(struct sensor *sensor)
{
        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        if (sd->phase) {
                // waiting for rising edge
                CMP0.daccr.vosel = 30;
                CMP0.scr.ief = false;
                CMP0.scr.ier = true;
        } else {
                // waiting for falling edge
                CMP0.daccr.vosel = 25;
                CMP0.scr.ier = false;
                CMP0.scr.ief = true;
        }

        gpio_write(sd->pin_a, sd->phase);

        FTM1.cnt = 0;
        FTM1.sc.clks = 0x1;
}

static void
cond_gpio_sample_done(struct sensor *sensor)
{

        struct cond_gpio_sensor_data *sd = sensor->sensor_data;
        #ifdef USE_VREF
        VREF.sc.raw = 0;
        #endif

        CMP0.cr1.en = 0;
        CMP0.daccr.dacen = 0;
        gpio_write(sd->pin_a, 0);

        pin_mode(PIN_PTC6, PIN_MODE_MUX_GPIO);
        gpio_dir(PIN_PTC6, GPIO_OUTPUT);
        gpio_write(PIN_PTC6, 0);

        accum dt_ms[2];
        for (int i=0; i<2; i++) {
                dt_ms[i] = 1000k / (48e6/128) * sd->t_accum[i] / (sd->transitions - 1);
                //accum cap_uf = 10;
                //accum r_kohm = dt_ms / cap_uf / log(1 - dv/vcc);
        }
        sensor_new_sample(sensor, dt_ms);
}

void
CMP0_Handler(void)
{
        struct cond_gpio_sensor_data *sd = _cond_sensor->sensor_data;

        FTM1.sc.clks = 0x0;
        CMP0.scr.raw |= ((struct CMP_SCR_t) {.cfr = 1, .cff = 1}).raw;

        /*
         * we need to throw out the first transition as it is measuring
         * the rise time from ground
         */
        if (sd->count < sd->transitions)
                sd->t_accum[sd->phase] += FTM1.cnt;

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
        sd->t_accum[0] = 0;
        sd->t_accum[1] = 0;

        FTM1.sc.ps = 7; // f/128
        FTM1.cntin = 0;
        FTM1.mod = 0xffff;

        pin_mode(PIN_PTC6, PIN_MODE_MUX_ANALOG);
        #ifdef USE_VREF
        SIM.scgc4.vref = 1;
        VREF.trm.chopen = 1;
        VREF.trm.trim = 0;
        VREF.sc.vrefen = 1;
        while (!VREF.sc.vrefst);
        VREF.sc.raw = ((struct VREF_SC_t) {.vrefen=1, .regen=1, .icompen=1, .mode_lv=2}).raw;
        CMP0.daccr.vrsel = 0;
        #else
        CMP0.daccr.vrsel = 1;
        #endif
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
        {.id = 1, .name = "conductivity", .unit = "arbitrary"},
};

struct sensor_type cond_gpio_sensor_type = {
        .sample_fn = &cond_gpio_sample,
        .no_stop = true,
        .n_measurables = 2,
        .measurables = cond_gpio_measurables
};
