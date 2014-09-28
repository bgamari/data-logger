#include <mchck.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "sensors/nmea.h"

/* This must not be called while a read is in flight */
static int
nmea_write(struct sensor *sensor, const char *fmt, ...)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        va_list args;
        va_start(args, fmt);
        int ret = snprintf(nmea->buffer, sizeof(nmea->buffer), fmt, args);
        va_end(args);
        if (ret < 0)
                return ret;

        uint8_t csum = 0;
        for (unsigned int i=1; i < ret; i++)
                csum ^= nmea->buffer[i];

        int ret2 = snprintf(&nmea->buffer[ret], sizeof(nmea->buffer) - ret,
                            "*%02x\r\n", csum);
        if (ret < 0)
                return ret;
        uart_write(nmea->uart, &nmea->trans, nmea->buffer, ret+ret2, NULL, NULL);
        return ret+ret2;
}

static void
nmea_mtk_power_save(struct sensor *sensor, bool pwr_save)
{
        nmea_write(sensor, "$PMTK320,%d", pwr_save); 
}

static void
nmea_set_enable(struct sensor *sensor, bool enabled)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        if (nmea->flavor == NMEA_MTK)
                nmea_mtk_power_save(sensor, !enabled);
        if (nmea->enable_pin)
                gpio_write(nmea->enable_pin, enabled);
}

// return first character after next comma (beginning of next field)
static const char *
next_field(const char *s)
{
        while (*s != ',') s++;
        return s + 1;
}

static bool
nmea_verify_checksum(const char *sentence)
{
        uint8_t csum = 0;
        if (*sentence != '$')
                return false;
        sentence++;
        while (*sentence != '*') {
                if (*sentence == '\n')
                        return true; // no checksum given, assume correct
                if (*sentence == 0)
                        return false; // premature end of sentence
                csum ^= *sentence;
                sentence++;
        }
        if (*sentence == '*') {
                char *end;
                uint8_t csum2 = strtol(sentence+1, &end, 16);
                if (*end != '\r')
                        return false;
                if (csum2 != csum)
                        return false;

        }
        return true;
}

static accum
parse_fixed(const char *s, char **next)
{
        char *point;
        accum whole = 1. * strtol(s, &point, 10);
        if (*point != '.') {
                if (next)
                        *next = point;
                return whole;
        }

        point++;
        char* tmp;
        accum frac = 1. * strtoul(point, &tmp, 10);
        while (tmp > point) {
                frac /= 10;
                point++;
        }
        if (next)
                *next = tmp;
        if (whole < 0)
                return whole - frac;
        else
                return whole + frac;
}

static void
handle_gpgga(struct sensor *sensor,
             accum lat, accum lon, accum alt, unsigned int nsats)
{
        struct nmea_sensor_data *sd = sensor->sensor_data;
        sd->values[0] = lat;
        sd->values[1] = lon;
        sd->values[2] = alt;
        sd->values[3] = nsats;
        sensor_new_sample(sensor, sd->values);
        nmea_set_enable(sensor, false);
}

/* Return true on successful parse of $GPGGA sentence */
static bool
nmea_parse_sentence(struct sensor *sensor, const char *sentence)
{
        const char *next = next_field(sentence);
        if (memcmp("$GPGGA", sentence, 6) == 0) {
                // next = time
                next = next_field(next);

                // next = lat
                accum lat = parse_fixed(next, NULL);
                next = next_field(next);
                // next = N/S
                if (*next == 'S') {
                        lat *= -1;
                } else if (*next != 'N') {
                        // parse error
                        return false;
                }

                next = next_field(next);
                // next = lat
                accum lon = parse_fixed(next, NULL);

                next = next_field(next);
                // next = E/W
                if (*next == 'E') {
                        lat *= -1;
                } else if (*next != 'W') {
                        // parse error
                        return false;
                }

                next = next_field(next);
                // next = GPS quality
                if (*next == 0) {
                        // no fix
                        return false;
                }

                next = next_field(next);
                // next = number of satellites
                unsigned int nsats = strtoul(next, NULL, 10);
                
                next = next_field(next);
                // next = horizontal precision

                next = next_field(next);
                // next = altitude
                accum alt = parse_fixed(next, NULL);
                next = next_field(next);
                // next = units
                
                // ignore everything else

                handle_gpgga(sensor, lat, lon, alt, nsats);
                return true;
        }

        return false;
}

void
nmea_init(struct sensor *sensor)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        uart_init(nmea->uart);
        uart_set_baudrate(nmea->uart, nmea->baudrate);

        if (nmea->enable_pin) {
                pin_mode(nmea->enable_pin, PIN_MODE_MUX_GPIO);
                gpio_dir(nmea->enable_pin, GPIO_OUTPUT);
                nmea_set_enable(sensor, false);
        }
}

static void nmea_read(struct sensor *sensor);

static void
nmea_data_available(const void *buf, size_t len, void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        bool done = false;
        
        /* find beginning of sentence */
        nmea->buffer[sizeof(nmea->buffer)-1] = 0; // null-terminate
        const char *sentence = nmea->buffer;
        while (sentence - nmea->buffer < sizeof(nmea->buffer)) {
                if (*sentence == '$') {
                        if (nmea_verify_checksum(sentence))
                                done = nmea_parse_sentence(sensor, sentence);
                        break;
                }
                sentence++;
        }

        if (!done)
                nmea_read(sensor);
}

static void
nmea_read(struct sensor *sensor)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        uart_read_until(nmea->uart, &nmea->trans,
                        nmea->buffer, sizeof(nmea->buffer), '\r',
                        nmea_data_available, sensor);
}

static void
nmea_sample_timeout(void *cbdata)
{
        struct sensor *sensor = cbdata;
        nmea_set_enable(sensor, false);
        sensor_sample_failed(sensor);
}

static void
nmea_sample(struct sensor *sensor)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        nmea_set_enable(sensor, true);
        bzero(nmea->buffer, sizeof(nmea->buffer)); // invalidate
        nmea_read(sensor);
        if (nmea->max_acquire_time)
                timeout_add(&nmea->timeout, 1000*nmea->max_acquire_time,
                            nmea_sample_timeout, sensor);
}

const char degrees[] = "degrees";

struct measurable nmea_gps_measurables[] = {
        {.id = 0, .name = "latitude", .unit = degrees},
        {.id = 1, .name = "longitude", .unit = degrees},
        {.id = 2, .name = "altitude", .unit = "meters"},
        {.id = 3, .name = "n-satellites", .unit = "count"},
};
        
struct sensor_type nmea_gps_sensor = {
        .sample_fn = nmea_sample,
        .no_stop = true,
        .n_measurables = 4,
        .measurables = nmea_gps_measurables
};
