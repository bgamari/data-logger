#include <mchck.h>
#include "acquire.h"

bool verbose = false;
bool acquire_running = false;

struct sample {
        uint32_t timestamp;
        accum temperature;
};

#define PAGE_SIZE 256
#define NSAMPLES (PAGE_SIZE / sizeof(struct sample))

struct spiflash_ctx spiflash;
static struct timeout_ctx timeout;

static unsigned int sample_idx = 0;
static unsigned int page_n = 0;
static struct sample sample_buffer[NSAMPLES];

static void
flash_program_cb(void *cbdata)
{}

static void
temp_done(uint16_t data, int error, void *cbdata)
{
        unsigned accum volt = adc_as_voltage(data);
        accum volt_diff = volt - 0.719k;
        accum temp_diff = volt_diff * (1000K / 1.715K);
        accum temp_deg = 25k - temp_diff;

        sample_buffer[sample_idx].timestamp = 0;
        sample_buffer[sample_idx].temperature = temp_deg;
        sample_idx++;
        if (verbose)
                printf("sample %d: temp=%.1k\n", sample_idx, temp_deg);
        
        if (sample_idx == NSAMPLES) {
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
        //start_blink(1, 200, 200);
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
        timeout_add(&timeout, 5000, timeout_cb, NULL);
        acquire_running = true;
}

void stop_acquire()
{
        timeout_cancel(&timeout);
        acquire_running = false;
}
