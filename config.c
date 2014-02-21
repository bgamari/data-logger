#include "config.h"
#include "acquire.h"

#include "sensors/thermistor.h"
#include "sensors/temperature.h"
#include "sensors/conductivity.h"
#include "sensors/nmea.h"
#include "sensors/bmp085_sensor.h"

// core temperature
struct sensor temperature_sensor = {
        .type = &core_temp_sensor_type,
        .name = "onboard-temperature",
        .sensor_id = 1,
        .sensor_data = &core_temperature_sensor_data
};

struct thermistor_map_data thermistor_map_data = {
        .beta = 3700,
        .t0 = 298,
        .r0 = 10e3,
        .r1 = 10e3,
        .polarity = 1
};

// thermistor #1
struct adc_sensor_data lm19_adc_sensor_data = {
        //.channel = ADC_PTD5,
        .channel = ADC_PTB3,
        .mode = ADC_MODE_AVG_32 | ADC_MODE_SAMPLE_LONG | ADC_MODE_POWER_LOW,
        .map = lm19_map,
        .map_data = &thermistor_map_data,
};
        
struct sensor thermistor_sensor = {
        .type = &thermistor_sensor_type,
        .name = "external temperature",
        .sensor_id = 2,
        .sensor_data = &lm19_adc_sensor_data,
};

// thermistor #2
/*
struct adc_sensor_data thermistor2_adc_sensor_data = {
        .channel = ADC_PTD6,
        .map = thermistor_map,
        .map_data = &thermistor_map_data,
};

struct sensor thermistor2_sensor = {
        .sample = &adc_sensor_sample,
        .name = "thermistor",
        .sensor_id = 3,
        .sensor_data = &thermistor2_adc_sensor_data,
};
*/

// conductivity sensor
struct cond_sensor_data cond_sensor_data = {
        .avg_count = 20,
};

struct sensor conductivity_sensor = {
        .type = &cond_sensor,
        .name = "conductivity",
        .sensor_id = 4,
        .sensor_data = &cond_sensor_data,
};

struct nmea_sensor_data gps_sensor_data = {
        .uart = &uart0,
        .enable_pin = 0,
        .baudrate = 57600
};

struct sensor gps_sensor = {
        .type = &nmea_gps_sensor,
        .name = "gps",
        .sensor_id = 5,
        .sensor_data = &gps_sensor_data
};

struct bmp085_sensor_data bmp085_sensor_data;

struct sensor bmp085_sensor = {
        .type = &bmp085_sensor_type,
        .name = "pressure",
        .sensor_id = 6,
        .sensor_data = &bmp085_sensor_data
};

struct sensor *sensors[] = {
        &temperature_sensor,
        &thermistor_sensor,
        //&thermistor2_sensor,
        //&conductivity_sensor,
        &gps_sensor,
        &bmp085_sensor,
        NULL
};

void
config_pins()
{
        pin_mode(PIN_PTB0, PIN_MODE_MUX_ANALOG);
        pin_mode(PIN_PTD5, PIN_MODE_MUX_ANALOG);
        //pin_mode(PIN_PTA1, PIN_MODE_MUX_ALT3 | PIN_MODE_PULLUP);
        //cond_init();

        nmea_init(&gps_sensor);
        pin_mode(PIN_PTA1, PIN_MODE_MUX_ALT2);
        pin_mode(PIN_PTA2, PIN_MODE_MUX_ALT2);
}
