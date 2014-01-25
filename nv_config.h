#include <mchck.h>

/*
 * non-volatile configuration data
 */
struct nv_config {
        uint32_t magic;
        bool acquire_on_boot;
        uint32_t sample_period;
        char name[32];
};

extern struct nv_config nv_config;

void nv_config_save(spi_cb cb, void *cbdata);

typedef void (*nv_config_loaded_cb)();

void nv_config_init(nv_config_loaded_cb cb);
