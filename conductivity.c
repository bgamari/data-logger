#include <mchck.h>
#include "sensor.h"
#include "conductivity.h"

static struct cond_sample_ctx *head = NULL;

static void
cond_start(void)
{
        if (FTM0.sc.clks)
                return;
        
        // TODO: bring up enable pin

        // start continuous dual-edge capture mode
        FTM0.sc.clks = 1; // enable clock
        FTM0.combine[0].decap = 1;
}

static void
cond_stop(void)
{
        // TODO: bring down enable pin
        FTM0.sc.clks = 0; // disable clock
}

void
cond_sample(struct cond_sample_ctx *ctx, cond_sample_cb cb, void *cbdata)
{
        // prevent multiple additions
        if (ctx->active) return;
        ctx->cb = cb;
        ctx->cbdata = cbdata;
        ctx->active = true;

        crit_enter();
        ctx->next = head;
        head = ctx;
        crit_exit();

        cond_start();
}

static bool
cond_average_cb(unsigned accum conductivity, void *cbdata)
{
        struct cond_average_ctx *ctx = cbdata;
        ctx->accumulator += conductivity;
        ctx->remaining--;
        if (ctx->remaining == 0) {
                unsigned accum mean = ctx->accumulator / ctx->nsamples;
                ctx->cb(mean, ctx->cbdata);
                return false;
        } else {
                return true;
        }
}

int
cond_average(struct cond_average_ctx *ctx, unsigned int n,
             cond_average_done_cb cb, void *cbdata)
{
        // Fail if there's already an averaging underway
        if (ctx->remaining > 0)
                return 1;

        ctx->accumulator = 0;
        ctx->nsamples = n;
        ctx->remaining = n;
        ctx->cb = cb;
        ctx->cbdata = cbdata;
        cond_sample(&ctx->ctx, cond_average_cb, ctx);
        return 0;
}

void
cond_init(void)
{
        // FIXME Enable pin
        //pin_mode();

        ftm_init();
        cond_stop();
        FTM0.sc.ps = FTM_PS_DIV32; // prescale
        FTM0.mode.ftmen = 1;
        int_enable(IRQ_FTM0);
        FTM0.combine[0].decapen = 1;

        // channel 0 captures falling edge
        FTM0.channel[0].csc.msa = 1;  // continuous
        FTM0.channel[0].csc.elsa = 0;
        FTM0.channel[0].csc.elsb = 1;
        FTM0.channel[0].csc.chf = 0;

        // channel 1 captures rising edge
        FTM0.channel[1].csc.elsa = 0;
        FTM0.channel[1].csc.elsb = 1;
        FTM0.channel[1].csc.chf = 0;
        FTM0.channel[1].csc.chie = 1;

        // PTC1 = pin 34 = FTM0_CH0
        FTM0.filter.ch0fval = 5;
        pin_mode(PIN_PTC1, PIN_MODE_MUX_ALT4); // | PIN_MODE_PULLDOWN);
}

#define UNUSED(expr) do { (void)(expr); } while (0)

void
FTM0_Handler(void)
{
        // acknowledge interrupt
        uint32_t a;
        UNUSED(a);
        a = FTM0.channel[0].csc.raw;
        FTM0.channel[0].csc.chf = 0;
        a = FTM0.channel[1].csc.raw;
        FTM0.channel[1].csc.chf = 0;

        // fetch edge times
        uint32_t t1 = FTM0.channel[0].cv;
        uint32_t t2 = FTM0.channel[1].cv;

        int32_t dt = t2 - t1;
        if (dt < 0) dt += 0xffff;
        uint32_t dt_us = dt / (48 / 32);
        unsigned accum conductivity = 1e-3 * dt_us; // FIXME

        struct cond_sample_ctx *ctx = head;
        struct cond_sample_ctx **last_next = &head;
        while (ctx != NULL) {
                bool more = (ctx->cb)(conductivity, ctx->cbdata);
                if (!more) {
                        *last_next = ctx->next;
                        ctx->active = false;
                } else {
                        last_next = &ctx->next;
                }
                ctx = ctx->next;
        }

        if (head == NULL)
                cond_stop();
}

static void
cond_sensor_sample_cb(unsigned accum conductivity, void* cbdata)
{
        struct sensor *sensor = cbdata;
        sensor_new_sample(sensor, conductivity);
}

void cond_sensor_sample(struct sensor *sensor)
{
        struct cond_sensor_data *sd = sensor->sensor_data;
        cond_average(&sd->ctx, 100, cond_sensor_sample_cb, sensor);
}
