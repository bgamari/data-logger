#pragma once

#include <stdbool.h>

extern struct spiflash_ctx spiflash;
extern bool acquire_running;

struct sample {
        uint32_t time;
        uint16_t sensor_id;
        uint16_t _reserved;
        accum value;
};

void acquire_init();
void set_sample_period(unsigned int milliseconds);
unsigned int get_sample_period();
void take_sample();
void start_acquire();
void stop_acquire();
bool acquire_can_stop();

// Reading back samples
int read_samples(struct spiflash_ctx *spiflash,
                 struct sample *buffer,
                 unsigned int start, unsigned int len,
                 spi_cb cb, void *cbdata);

void acquire_blink_state();
