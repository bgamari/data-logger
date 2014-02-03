#include <mchck.h>
#include <nrf/nrf.h>

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "version.h"
#include "power.h"
#include "acquire.h"
#include "sensor.h"
#include "blink.h"
#include "conductivity.h"
#include "usb_console.h"
#include "nv_config.h"
#include "sample_store.h"
#include "radio.h"

/*
 * command processing
 */
static char cmd_buffer[32];
static volatile bool command_queued = false;

#define OUT usb_console_printf
static void
reply_finished(void *cbdata)
{
        command_queued = false;
}

static void
finish_reply()
{
        usb_console_printf("\n");
        usb_console_flush(reply_finished, NULL);
}


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
                OUT("cond: %3.4k\n", mean);
                usb_console_flush(NULL, NULL);
                acc = 0;
                acc_i = acc_n;
        }
        return dumping_conductivity;
}

/*
 * live sensor sample dumping
 */
static volatile bool verbose = false;
struct sensor_listener listener;

static void
on_sample_cb(struct sensor *sensor, accum value, void *cbdata)
{
        if (verbose)
                printf("%d     %3.3k\n", sensor->sensor_id, value);
}

/*
 * stored sample printing
 */
static struct sample sample_buffer;
static volatile unsigned int sample_idx = 0;
static volatile unsigned int sample_end = 0;
static struct spiflash_transaction sample_read_trans;

static void print_stored_sample(void *cbdata);

static void
print_sample(struct sample *sample)
{
        usb_console_printf("%10d    %2d    %2.3k\n",
               sample->time, sample->sensor_id, sample->value);
}

static void
get_stored_sample(void *cbdata)
{
        int ret = sample_store_read(&sample_read_trans,
                                    &sample_buffer, sample_idx, 1,
                                    print_stored_sample, NULL);
        if (ret != 0) {
                OUT("error\n");
                finish_reply();
        }
}
        
static void
print_stored_sample(void *cbdata)
{
        print_sample(&sample_buffer);
        if (sample_idx < sample_end - 1) {
                sample_idx++;
                usb_console_flush(get_stored_sample, NULL);
        } else {
                finish_reply();
        }
}

/*
 * SPI flash commands
 */ 
static void
flash_id_cb(void *cbdata, uint8_t mfg_id, uint8_t memtype, uint8_t capacity)
{
        OUT("flash: mfg=%x memtype=%x capacity=%x\n",
               mfg_id, memtype, capacity);
        finish_reply();
}

static void
flash_status_cb(void *cbdata, uint8_t status)
{
        OUT("flash: status=%02x\n", status);
        finish_reply();
}

// `s` command: list sensors
static void
print_sensor(void *cbdata)
{
        struct sensor **s = cbdata;
        if (*s == NULL) {
                finish_reply();
        } else {
                OUT("%2d    %15s    %10s\n",
                    (*s)->sensor_id,
                    (*s)->name ? (*s)->name : "unknown",
                    (*s)->unit ? (*s)->unit : "unknown");
                usb_console_flush(print_sensor, s+1);
        }
}

// `l` command: last sensor value
static void
last_sensor_sample(void *cbdata)
{
        struct sensor **s = cbdata;
        if (*s == NULL) {
                finish_reply();
        } else {
                OUT("%10d    %2d    %2.3k\n", (*s)->last_sample_time,
                    (*s)->sensor_id,
                    (*s)->last_sample);
                usb_console_flush(last_sensor_sample, s+1);
        }
}

// `R` command: recovery last sample index
static void
recovery_done()
{
        OUT("sample count = %d\n", sample_store_get_count());
        finish_reply();
}

static void
nv_configuration_saved(void *cbdata)
{
        OUT("saved\n");
        finish_reply();
}

struct spiflash_transaction get_id_transaction;

static void
process_command()
{
        const char *data = cmd_buffer;
        switch (data[0]) {
        case 'a':     // acquisition status
                if (data[1] == '=') {
                        if (data[2] == '0')
                                stop_acquire();
                        else
                                start_acquire();
                }
                OUT("acquiring = %d\n", acquire_running);
                finish_reply();
                break;
        case 'f':     // force a sample
                take_sample();
                finish_reply();
                break;
        case 'v':     // enable verbose mode (dump every sample)
                if (data[1] == '=')
                        verbose = data[2] == '1';
                OUT("verbose = %d\n", verbose);
                finish_reply();
                break;
        case 'r':     // enable radio report mode (dump every sample)
                if (data[1] == '=') {
                        if (data[2] == '1')
                                radio_enable();
                        else
                                radio_disable();
                }
                OUT("radio = %d\n", radio_get_enabled());
                finish_reply();
                break;
        case 'F':     // identify FLASH device
                if (data[1] == 'i') {
                        spiflash_get_id(&onboard_flash, &get_id_transaction,
                                        flash_id_cb, NULL);
                } else if (data[1] == 's') {
                        spiflash_get_status(&onboard_flash, &get_id_transaction,
                                            flash_status_cb, NULL);
                } else {
                        OUT("unknown command\n");
                        finish_reply();
                }
                break;
        case 'g':     // get stored samples
        {
                char *pos;
                sample_idx = strtoul(&data[2], &pos, 10);
                sample_end = sample_idx + strtoul(&pos[1], NULL, 10);
                if (sample_end - sample_idx > 0)
                        get_stored_sample(NULL);
                else
                        finish_reply();
                break;
        }
        case 'c':     // dump conductivity samples
                if (!dumping_conductivity) {
                        dumping_conductivity = true;
                        cond_sample(&cond_sample_ctx, cond_new_sample_cb, NULL);
                } else {
                        dumping_conductivity = false;
                }
                OUT("go\n");
                finish_reply();
                break;
        case 't':     // RTC time
                if (data[1] == '=') {
                        uint32_t time = strtoul(&data[2], NULL, 10);
                        rtc_set_time(time);
                        rtc_start_counter();
                }
                OUT("RTC time = %lu\n", rtc_get_time());
                finish_reply();
                break;
        case 'T':     // sample period in milliseconds
                if (data[1] == '=') {
                        uint32_t time = strtoul(&data[2], NULL, 10);
                        if (time > 0) {
                                set_sample_period(time);
                                nv_config.sample_period = time;
                        }
                }
                OUT("sample period = %d\n", get_sample_period());
                finish_reply();
                break;
        case 's':     // list sensors
                print_sensor(sensors);
                break;
        case 'l':     // list last sensor values
                last_sensor_sample(sensors);
                break;
        case 'n':     // fetch stored sample count
                if (data[1] == '!') {
                        sample_store_reset();
                }
                OUT("sample count = %d\n", sample_store_get_count());
                finish_reply();
                break;
        case 'N':     // friendly name of device
                if (data[1] == '=') {
                        strncpy(nv_config.name, &data[2], sizeof(nv_config.name));
                }
                OUT("device name = %s\n", &nv_config.name);
                finish_reply();
                break;
        case 'V':     // fetch firmware version
                OUT("version = %s\n", commit_id);
                finish_reply();
                break;
        case 'I':     // fetch device ID
                OUT("device id = %x-%x-%x-%x\n",
                    SIM.uidl, SIM.uidml, SIM.uidmh, SIM.uidh);
                finish_reply();
                break;
        case 'p':     // enter power-save mode
                if (data[1] == '!') {
                        OUT("powersave = 1\n");
                        finish_reply();
                        enter_low_power_mode();
                }
                break;
        case 'B':     // acquire-on-boot flag
                if (data[1] == '=')
                        nv_config.acquire_on_boot = data[2] == '1';
                OUT("acquire on boot = %d\n", nv_config.acquire_on_boot);
                finish_reply();
                break;
        case 'S':     // save non-volatile configuration
                nv_config_save(nv_configuration_saved, NULL);
                break;
        case 'R':     // recover from power loss
                sample_store_recover(recovery_done);
                break;
        default:
                OUT("unknown command\n");
                finish_reply();
        }
}

static void
console_line_recvd(const char *cmd, size_t len)
{
        // notify power-management of activity
        power_notify_activity();
        
        if (cmd[0] == '!') {
                usb_console_reset();
                command_queued = false;
                return;
        }
        
        if (command_queued)
                return;
        strncpy(cmd_buffer, cmd, sizeof(cmd_buffer));
        command_queued = true;
        process_command();
}

static bool usb_on = true;

static void
nv_config_available()
{
        // begin acquiring if so-configured
        if (nv_config.acquire_on_boot) {
                set_sample_period(nv_config.sample_period);
                sample_store_recover(start_acquire);
        }
}

void
main(void)
{
        config_pins();
        power_init();
        adc_init();
        spi_init();
        spiflash_pins_init();
        timeout_init();
        rtc_init();
        sample_store_init();
        usb_console_line_recvd_cb = console_line_recvd;
        usb_console_init();
        cond_init();
        acquire_init();
        radio_init();
        sensor_listen(&listener, on_sample_cb, NULL);
        start_blink(5, 100, 100);

        // configure LLWU
        // enable RTC alarm wake-up source
        LLWU.wume |= 1 << 5;
        // enable LLWU_P3 = PTA4 as wake-up source
        pin_mode(PIN_PTA4, PIN_MODE_MUX_ALT1);
        LLWU.wupe[0].wupe3 = LLWU_PE_FALLING;
        LLWU.filt1.filtsel = 3; // P3
        LLWU.filt1.filte = LLWU_FILTER_BOTH;

        // load non-volatile configuration
        nv_config_init(nv_config_available);

        // event loop
        while (true) {
                bool can_deep_sleep = low_power_mode && spi_is_idle();
                SCB.scr.sleepdeep = can_deep_sleep;
                if (can_deep_sleep) {
                        // TODO: power things down
                        if (usb_on) {
                                USB0.ctl.raw |= ((struct USB_CTL_t){
                                                .txd_suspend = 1,
                                                        .usben = 0,
                                                        .oddrst = 1,
                                                        }).raw;
                                SIM.scgc4.usbotg = 0;
                                usb_on = false;
                        }
                }
                __asm__("wfi");
                if (SMC.pmctrl.stopa)
                        onboard_led(1);
        }
}
