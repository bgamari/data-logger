#pragma once
#include <stdbool.h>

extern struct spiflash_ctx spiflash;
extern bool verbose;
extern bool acquire_running;

void take_sample();
void start_acquire();
