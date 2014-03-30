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
#include "sensors/conductivity.h"
#include "usb_console.h"
#include "nv_config.h"
#include "sample_store.h"
#include "radio.h"
#include "sensor_iter.h"

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
 * live sensor sample dumping
 */
static volatile bool verbose = false;
struct sensor_listener listener;

static void
on_sample_cb(struct sensor *sensor, uint32_t time,
             uint8_t measurable, accum value, void *cbdata)
{
        if (verbose)
                printf("%d     %d     %3.4k\n", sensor->sensor_id, measurable, value);
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
print_sample(const struct sample *sample)
{
        usb_console_printf("%10d    %2d    %2d    %4.4k\n",
                           sample->time, sample->sensor_id,
                           sample->measurable, sample->value);
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
                OUT("%2d\t%20s\n",
                    (*s)->sensor_id,
                    (*s)->name);
                usb_console_flush(print_sensor, s+1);
        }
}

// `m` command: list measurables of sensor
struct list_measurables_state {
        struct sensor *sensor;
        unsigned int measurable_idx;
} list_measurables_state;

static void
print_measurable(void *cbdata)
{
        struct sensor *s = list_measurables_state.sensor;
        if (list_measurables_state.measurable_idx < s->type->n_measurables) {
                struct measurable *m = &s->type->measurables[list_measurables_state.measurable_idx];
                OUT("%2d\t%20s\t%20s\n",
                    m->id,
                    m->name ? m->name : "unknown",
                    m->unit ? m->unit : "unknown");
                list_measurables_state.measurable_idx++;
                usb_console_flush(print_measurable, NULL);
        } else {
                finish_reply();
        }
}

// `l` command: last sensor value
struct measurable_iterator last_sample_iter;

static void
last_sensor_sample(void *cbdata)
{
        if (meas_iter_next(&last_sample_iter)) {
                struct sensor *s = meas_iter_get_sensor(&last_sample_iter);
                struct measurable *m = meas_iter_get_measurable(&last_sample_iter);
                accum value = sensor_get_value(s, m->id);
                OUT("%d    %10d    %2d    %2.4k\n",
                    s->last_sample_time,
                    s->sensor_id,
                    m->id,
                    value);
                usb_console_flush(last_sensor_sample, s+1);
        } else {
                finish_reply();
        }
}

static void
nv_configuration_saved(void *cbdata)
{
        OUT("saved\n");
        finish_reply();
}

static void
nv_configuration_reloaded(void *cbdata)
{
        OUT("loaded\n");
        finish_reply();
}

struct spiflash_transaction get_id_transaction;

static void
process_command()
{
        bool unknown = false;
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
                        set_sample_period(time);
                        nv_config.sample_period = get_sample_period();
                }
                OUT("sample period = %d\n", get_sample_period());
                finish_reply();
                break;
        case 's':     // list sensors
                print_sensor(sensors);
                break;
        case 'm':     // list measurables
        {
                uint32_t sensor_id = strtoul(&data[2], NULL, 10);
                struct sensor **s=sensors;
                while (*s) {
                        if ((*s)->sensor_id == sensor_id) {
                                list_measurables_state.sensor = *s;
                                list_measurables_state.measurable_idx = 0;
                                print_measurable(NULL);
                                break;
                        }
                        s++;
                }
                if (*s == NULL) {
                        OUT("unknown sensor\n");
                        finish_reply();
                }
                break;
        }
        case 'l':     // list last sensor values
                meas_iter_init(&last_sample_iter);
                last_sensor_sample(sensors);
                break;
        case 'n':     // fetch stored sample count
                if (data[1] == '!') {
                        sample_store_reset();
                }
                OUT("sample count = %d\n", sample_store_get_count());
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
        case 'N':
                switch (data[1]) {
                case 'B':     // acquire-on-boot flag
                        if (data[2] == '=')
                                nv_config.acquire_on_boot = data[3] == '1';
                        OUT("acquire on boot = %d\n", nv_config.acquire_on_boot);
                        finish_reply();
                        break;
                case 'N':     // friendly name of device
                        if (data[2] == '=') {
                                strncpy(nv_config.name, &data[3], sizeof(nv_config.name));
                        }
                        OUT("device name = %s\n", &nv_config.name);
                        finish_reply();
                        break;
                case 'S':     // save non-volatile configuration
                        nv_config_save(nv_configuration_saved, NULL);
                        break;
                case 'R':     // reload non-volatile configuration
                        nv_config_reload(nv_configuration_reloaded);
                        break;
                default:
                        unknown = true;
                }
                break;
        default:
                unknown = true;
        }

        if (unknown) {
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
                start_acquire();
        }
}

void
main(void)
{
        config_pins();
        power_init();
        spi_init();
        spiflash_pins_init();
        pin_change_init();
        timeout_init();
        rtc_init();
        sample_store_init();
        usb_console_line_recvd_cb = console_line_recvd;
        usb_console_init();
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
        nv_config_reload(nv_config_available);

        // event loop
        while (true) {
                bool can_deep_sleep = low_power_mode
                        && spi_is_idle() && acquire_can_stop();
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
