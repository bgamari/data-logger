#include <mchck.h>
#include "blink.h"
#include "acquire.h"

bool verbose = false;
bool acquire_running = false;

struct spiflash_ctx spiflash;
static struct timeout_ctx timeout;

static unsigned int sample_idx = 0;

#define PAGE_SIZE 256
#define SAMPLES_PER_PAGE (PAGE_SIZE / sizeof(struct sample))
#define SECTOR_SIZE 4096
#define SAMPLES_PER_SECTOR (SECTOR_SIZE / sizeof(struct sample))

// start and len given in samples
int
read_samples(struct spiflash_ctx *spiflash,
             struct sample *buffer,
             unsigned int start, unsigned int len,
             spi_cb cb, void *cbdata)
{
        return spiflash_read_page(spiflash, (uint8_t*) buffer,
                                  start * sizeof(struct sample),
                                  len * sizeof(struct sample),
                                  cb, cbdata);
}

struct write_sample {
        struct spiflash_ctx spiflash;
        struct sample sample;
        bool pending;
        uint32_t addr;
};

#define N_WRITE_SAMPLES 4
static struct write_sample write_samples[N_WRITE_SAMPLES];

static void
sample_written(void *cbdata)
{
        struct write_sample *w = cbdata;
        w->pending = false;
}

static int
write_sample(struct write_sample *w)
{
        return spiflash_program_page(&w->spiflash, w->addr,
                                     (const uint8_t*) &w->sample, sizeof(struct sample),
                                     sample_written, w);
}

static void
sector_erased(void *cbdata)
{
        write_sample(cbdata);
}

static int
push_sample(const struct sample s)
{
        // find available write_sample
        struct write_sample *w = NULL;
        crit_enter();
        for (unsigned int i=0; i<N_WRITE_SAMPLES; i++) {
                if (!write_samples[i].pending)
                        w = &write_samples[i];
        }
        if (w == NULL) {
                crit_exit();
                return 1;
        } else {
                w->pending = true;
                crit_exit();
        }

        w->sample = s;
        w->addr = sample_idx * sizeof(struct sample);

        int ret;
        if (sample_idx % SAMPLES_PER_SECTOR == 0) {
                // erase if starting new sector
                ret = spiflash_erase_sector(&w->spiflash, w->addr, sector_erased, w);
        } else {
                ret = write_sample(w);
        }

        sample_idx++;
        if (verbose)
                printf("sample %d: temp=%.1k\n", sample_idx, s.temperature);
        return ret;
}

static void
temp_done(uint16_t data, int error, void *cbdata)
{
        unsigned accum volt = adc_as_voltage(data);
        accum volt_diff = volt - 0.719k;
        accum temp_diff = volt_diff * (1000K / 1.715K);
        accum temp_deg = 25k - temp_diff;

        push_sample((struct sample) {
                        .type = TIME,
                        .time = rtc_get_time()
                    });
        push_sample((struct sample) {
                        .type = TEMPERATURE,
                        .time = temp_deg
                    });
}

void
take_sample()
{
        start_blink(1, 50, 50);
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
        start_blink(2, 50, 50);
        timeout_add(&timeout, 1000, timeout_cb, NULL);
        acquire_running = true;
}

void stop_acquire()
{
        start_blink(3, 50, 50);
        timeout_cancel(&timeout);
        acquire_running = false;
}
