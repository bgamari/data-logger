#include <mchck.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "version.h"
#include "acquire.h"
#include "sensor.h"
#include "blink.h"
#include "conductivity.h"
#include "usb_console.h"
#include "sample_store.h"

/*
 * command processing
 */
#define OUT usb_console_printf_blocking
static char cmd_buffer[32];
static volatile bool command_queued = false;

/*
 * conductivity dumping
 */
static struct cond_sample_ctx cond_sample_ctx;
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

/*
 * sensor sample dumping
 */
static volatile bool verbose = false;
struct sensor_listener listener;

static void
on_sample_cb(struct sensor *sensor, accum value, void *cbdata)
{
        if (verbose)
                printf("%d     %.1k\n", sensor->sensor_id, value);
}

/*
 * stored sample printing
 */
static struct sample sample_buffer;
static volatile bool sample_valid;
static struct sample_store_read_ctx sample_read_ctx;

static void
print_sample(struct sample *sample)
{
        OUT("%10d    %2d    %2.3k\n",
               sample->time, sample->sensor_id, sample->value);
}

static void
print_stored_sample(void *cbdata)
{
        sample_valid = true;
}

/*
 * SPI flash identification
 */ 
static void
spiflash_id_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        OUT("flash: mfg=%x memtype=%x capacity=%x\n\n",
               mfg_id, memtype, capacity);
        command_queued = false;
}

static volatile bool power_save_mode = false;
struct spiflash_transaction get_id_transaction;

static void
process_command()
{
        const char *data = cmd_buffer;
        switch (data[0]) {
        case 'a':
                if (data[1] == '=') {
                        if (data[2] == '0')
                                stop_acquire();
                        else
                                start_acquire();
                }
                OUT("acquiring = %d\n\n", acquire_running);
                command_queued = false;
                break;
        case 'f':
                take_sample();
                OUT("\n");
                command_queued = false;
                break;
        case 'v':
                if (data[1] == '=')
                        verbose = data[2] == '1';
                OUT("verbose = %d\n\n", verbose);
                command_queued = false;
                break;
        case 'i':
                spiflash_get_id(&onboard_flash, &get_id_transaction, spiflash_id_cb, NULL);
                break;
        case 'g':
        {
                char *pos;
                unsigned int idx = strtoul(&data[2], &pos, 10);
                unsigned int end = idx + strtoul(&pos[1], NULL, 10);
                while (idx < end) {
                        sample_valid = false;
                        int ret = sample_store_read(&sample_read_ctx,
                                                    &sample_buffer, idx, 1,
                                                    print_stored_sample, NULL);
                        if (ret != 0) {
                                OUT("error\n\n");
                                command_queued = false;
                                break;
                        }
                                
                        while (!sample_valid) __asm__("wfi");
                        print_sample(&sample_buffer);
                        idx++;
                } 
                OUT("\n");
                command_queued = false;
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
                OUT("RTC time = %lu\n\n", rtc_get_time());
                command_queued = false;
                break;
        case 'T':
                if (data[1] == '=') {
                        uint32_t time = strtoul(&data[2], NULL, 10);
                        set_sample_period(time);
                }
                OUT("sample period = %d\n\n", get_sample_period());
                command_queued = false;
                break;
        case 's':
                for (struct sensor **s = &sensors[0]; *s != NULL; s++)
                        OUT("%2d    %15s    %10s\n",
                            (*s)->sensor_id,
                            (*s)->name ? (*s)->name : "unknown",
                            (*s)->unit ? (*s)->unit : "unknown");
                OUT("\n");
                command_queued = false;
                break;
        case 'l':
                for (struct sensor **s = &sensors[0]; *s != NULL; s++)
                        OUT("%10d    %2d    %2.3k\n", (*s)->last_sample_time,
                            (*s)->sensor_id,
                            (*s)->last_sample);
                OUT("\n");
                command_queued = false;
                break;
        case 'n':
                OUT("sample count = %d\n\n", sample_store_get_count());
                command_queued = false;
                break;
        case 'V':
                OUT("version = %s\n\n", commit_id);
                command_queued = false;
                break;
        case 'I':
                OUT("device id = %x-%x-%x-%x\n\n",
                    SIM.uidl, SIM.uidml, SIM.uidmh, SIM.uidh);
                command_queued = false;
                break;
        case 'p':
                if (data[1] == 'p') {
                        OUT("powersave\n\n");
                        // usb_disable(); // FIXME
                        power_save_mode = true;
                }
                command_queued = false;
                break;
        default:
                OUT("unknown command\n\n");
                command_queued = false;
        }
}

static void
console_line_recvd(const char *cmd, size_t len)
{
        if (command_queued)
                return;
        strncpy(cmd_buffer, cmd, sizeof(cmd_buffer));
        command_queued = true;
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
        usb_console_line_recvd_cb = console_line_recvd;
        usb_console_init();
        cond_init();
        acquire_init();
        sensor_listen(&listener, on_sample_cb, NULL);
        start_blink(5, 100, 100);

        // configure LLWU
        // enable RTC alarm wake-up source
        LLWU.wume |= 0x5;
        // enable LLWU_P3 = PTA4 as wake-up source
        pin_mode(PIN_PTA4, PIN_MODE_MUX_ALT1);
        LLWU.wupe[0].wupe3 = LLWU_PE_FALLING;
        LLWU.filt1.filtsel = 3; // P3
        LLWU.filt1.filte = LLWU_FILTER_BOTH;

        // event loop
        while (true) {
                bool can_deep_sleep = power_save_mode
                        && spiflash_is_idle(&onboard_flash);
                SCB.scr.sleepdeep = can_deep_sleep;
                if (can_deep_sleep) {
                        // TODO: power things down
                }
                __asm__("wfi");

                if (command_queued)
                        process_command();
        }
}
