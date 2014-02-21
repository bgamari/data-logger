#include <mchck.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "sensors/nmea.h"

static void
nmea_set_enable(struct sensor *sensor, bool enabled)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
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
        sensor_new_sample(sensor, 0, lat, 1, lon, 2, alt, 3, nsats);
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
nmea_data_available(void *cbdata)
{
        struct sensor *sensor = cbdata;
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        bool done = false;
        
        /* find beginning of sentence */
        nmea->buffer[sizeof(nmea->buffer)] = 0; // null-terminate
        const char *sentence = nmea->buffer;
        while (sentence - nmea->buffer < sizeof(nmea->buffer)) {
                if (*sentence == '$') {
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
nmea_sample(struct sensor *sensor)
{
        struct nmea_sensor_data *nmea = sensor->sensor_data;
        nmea_set_enable(sensor, true);
        bzero(nmea->buffer, sizeof(nmea->buffer)); // invalidate
        nmea_read(sensor);
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

