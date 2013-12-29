#pragma once

#include <stdbool.h>

extern struct spiflash_ctx spiflash;
extern bool verbose;
extern bool acquire_running;

struct sample {
        uint32_t timestamp;
        accum temperature;
};

void take_sample();
void start_acquire();
void stop_acquire();

// Reading back samples
int read_samples(struct spiflash_ctx *spiflash,
                 struct sample *buffer,
                 unsigned int start, unsigned int len,
                 spi_cb cb, void *cbdata);
