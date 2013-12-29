#pragma once

#include <stdbool.h>

extern struct spiflash_ctx spiflash;
extern bool verbose;
extern bool acquire_running;

struct sample {
        uint32_t timestamp;
        accum temperature;
};

#define PAGE_SIZE 256
#define SAMPLES_PER_PAGE (PAGE_SIZE / sizeof(struct sample))
#define SECTOR_SIZE 4096
#define SAMPLES_PER_SECTOR (SECTOR_SIZE / sizeof(struct sample))

void take_sample();
void start_acquire();
void stop_acquire();

// Reading back samples
typedef void (*read_sample_cb)(struct sample sample, void *cbdata);

struct read_samples_ctx {
        struct spiflash_ctx spiflash;
        read_sample_cb cb;
        void *cbdata;
        unsigned int remaining;  // in samples
        unsigned int idx;        // in samples
        struct sample buffer[SAMPLES_PER_PAGE];
};
int read_samples(struct read_samples_ctx *ctx,
                 unsigned int start, unsigned int desired,
                 read_sample_cb cb, void *cbdata);
