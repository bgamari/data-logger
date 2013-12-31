#include <mchck.h>
#include <stdlib.h>

#include "config.h"
#include "acquire.h"
#include "sensor.h"
#include "blink.h"
#include "conductivity.h"
#include "usb_console.h"
#include "sample_store.h"

static struct cond_sample_ctx cond_sample_ctx;

/*
 * conductivity dumping
 */
static bool dumping_conductivity = false;
static unsigned accum acc = 0;
static unsigned int acc_n = 10;
static unsigned int acc_i = 10;

static bool cond_new_sample_cb(unsigned accum conductivity, void *cbdata)
{
        acc += conductivity;
        acc_i--;
        if (acc_i == 0) {
                unsigned accum mean = acc / acc_n;
                printf("cond: %3.4k\n", mean);
                acc = 0;
                acc_i = acc_n;
        }
        return dumping_conductivity;
}

static bool verbose = false;
struct sensor_listener listener;

static void
on_sample_cb(struct sensor *sensor, accum value, void *cbdata)
{
        if (verbose)
                printf("%d     %.1k\n", sensor->sensor_id, value);
}

/*
 * Sample printing
 */
unsigned int print_sample_idx = 0;
unsigned int print_sample_remaining = 0;
static struct sample sample_buffer;

static void
print_sample(void *cbdata)
{
        printf("%5d    %d    %2.3k\n",
               sample_buffer.time, sample_buffer.sensor_id, sample_buffer.value);
        if (print_sample_remaining > 0) {
                print_sample_remaining--;
                print_sample_idx++;
                sample_store_read(&sample_buffer,
                                  print_sample_idx, 1,
                                  print_sample, NULL);
        }
}

/*
 * SPI flash identification
 */ 
static void
spiflash_id_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        printf("flash: mfg=%x memtype=%x capacity=%x\n", mfg_id, memtype, capacity);
}

unsigned int read_offset = 0;

static void
process_command_cb(char *data, size_t len)
{
        switch (data[0]) {
        case 's':
                if (!acquire_running)
                        start_acquire();
                break;
        case 'S':
                if (acquire_running)
                        stop_acquire();
                break;
        case 'f':
                take_sample();
                break;
        case 'v':
                if (data[1] == '=')
                        verbose = data[2] == '1';
                printf("verbose = %d\n", verbose);
                break;
        case 'i':
                spiflash_get_id(&spiflash, spiflash_id_cb, NULL);
                break;
        case 'g':
        {
                char *end;
                print_sample_idx = strtoul(&data[2], &end, 10);
                print_sample_remaining = strtoul(&end[1], NULL, 10);
                print_sample(NULL);
                break;
        }
        case 'c':
                if (!dumping_conductivity) {
                        dumping_conductivity = true;
                        cond_sample(&cond_sample_ctx, cond_new_sample_cb, NULL);
                } else {
                        dumping_conductivity = false;
                }
                break;
        case 't':
                if (data[1] == '=') {
                        uint32_t time = strtoul(&data[2], NULL, 10);
                        rtc_set_time(time);
                        rtc_start_counter();
                }
                printf("RTC time = %d\n", RTC.tsr);
                break;
        case 'T':
                if (data[1] == '=') {
                        uint32_t time = strtoul(&data[2], NULL, 10);
                        set_sample_period(time);
                }
                printf("sample period = %d\n", get_sample_period());
                break;
        case 'n':
                printf("%d\n", sample_store_get_count());
                break;
        default:
                printf("Unknown command\n");
        }
}

void
main(void)
{
        config_pins();
        adc_init();
        spi_init();
        spiflash_pins_init();
        timeout_init();
        rtc_init();
        usb_console_line_recvd_cb = process_command_cb;
        usb_console_init();
        cond_init();
        start_blink(5, 100, 100);
        acquire_init();
        sensor_listen(&listener, on_sample_cb, NULL);
        adc_sample_prepare(ADC_MODE_POWER_LOW | ADC_MODE_SAMPLE_LONG | ADC_MODE_AVG_32);
        sys_yield_for_frogs();
}
