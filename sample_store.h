#pragma once
#include <mchck.h>
#include <acquire.h>

int sample_store_read(struct sample *buffer,
                      unsigned int start, unsigned int len,
                      spi_cb cb, void *cbdata);

int sample_store_push(const struct sample s);

unsigned int sample_store_get_count();
