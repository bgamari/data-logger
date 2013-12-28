#include <mchck.h>
#include "blink.h"
#include "acquire.h"

bool verbose = false;
bool acquire_running = false;

struct spiflash_ctx spiflash;
static struct timeout_ctx timeout;

static unsigned int sample_idx = 0;
static unsigned int page_n = 0;
static struct sample sample_buffer[SAMPLES_PER_PAGE];

static void
read_page_cb(void *cbdata)
{
        struct read_samples_ctx *ctx = cbdata;
        unsigned int i = 0;
        while (ctx->remaining > 0 && i < SAMPLES_PER_PAGE) {
                ctx->cb(ctx->buffer[i], ctx->cbdata);
                ctx->remaining--;
                i++;
        }
        ctx->idx += i;

        if (ctx->remaining > 0) {
                // TODO: Handle error
                spiflash_read_page(&ctx->spiflash, (uint8_t*) ctx->buffer,
                                   ctx->idx / SAMPLES_PER_PAGE, PAGE_SIZE,
                                   read_page_cb, ctx);
        }
}

int
read_samples(struct read_samples_ctx *ctx,
             unsigned int start, unsigned int desired,
             read_sample_cb cb, void *cbdata)
{
        ctx->cb        = cb;
        ctx->cbdata    = cbdata;
        ctx->idx       = start;
        ctx->remaining = desired;
        return spiflash_read_page(&ctx->spiflash, (uint8_t*) ctx->buffer,
                                  ctx->idx / SAMPLES_PER_PAGE, PAGE_SIZE,
                                  read_page_cb, ctx);
}

static void
flash_program_cb(void *cbdata)
{}

static uint32_t time = 0;
static void
temp_done(uint16_t data, int error, void *cbdata)
{
        unsigned accum volt = adc_as_voltage(data);
        accum volt_diff = volt - 0.719k;
        accum temp_diff = volt_diff * (1000K / 1.715K);
        accum temp_deg = 25k - temp_diff;

        sample_buffer[sample_idx].timestamp = time++;
        sample_buffer[sample_idx].temperature = temp_deg;
        sample_idx++;
        if (verbose)
                printf("sample %d: temp=%.1k\n", sample_idx, temp_deg);
        
        if (sample_idx == SAMPLES_PER_PAGE) {
                int ret = spiflash_program_page(&spiflash, page_n,
                                                (const uint8_t*) sample_buffer, 1,
                                                flash_program_cb, NULL);
                if (verbose)
                        printf("write page=%d: %d\n", page_n, ret);
                page_n++;
                sample_idx = 0;
        }
}

void
take_sample()
{
        start_blink(1, 100, 100);
        adc_sample_start(ADC_TEMP, temp_done, NULL);
}

static void
timeout_cb(void *data)
{
        take_sample();
        timeout_add(&timeout, 500, timeout_cb, NULL);
}

void start_acquire()
{
        start_blink(2, 100, 100);
        timeout_add(&timeout, 1000, timeout_cb, NULL);
        acquire_running = true;
}

void stop_acquire()
{
        start_blink(1, 100, 100);
        timeout_cancel(&timeout);
        acquire_running = false;
}
