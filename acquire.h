#pragma once

#include <stdbool.h>

extern struct spiflash_ctx spiflash;
extern bool verbose;
extern bool acquire_running;

struct sample {
        enum sample_type {
                TIME,
                TEMPERATURE,
                CONDUCTIVITY
        } type;
        union {
                uint32_t time;
                accum temperature;
                unsigned accum conductivity;
        };
};

void take_sample();
void start_acquire();
void stop_acquire();

// Reading back samples
int read_samples(struct spiflash_ctx *spiflash,
                 struct sample *buffer,
                 unsigned int start, unsigned int len,
                 spi_cb cb, void *cbdata);
