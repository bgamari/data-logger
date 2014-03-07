#include "config.h"
#include "acquire.h"

#include "sensors/thermistor.h"
#include "sensors/core_temp.h"
#include "sensors/lm19.h"
#include "sensors/conductivity.h"
#include "sensors/nmea.h"
#include "sensors/bmp085_sensor.h"
#include "sensors/flow.h"
#include "sensors/battery_voltage.h"
#include "sensors/tcs3472_sensor.h"

// core temperature
struct sensor core_temp_sensor = {
        .type = &core_temp_sensor_type,
        .name = "core temperature",
        .sensor_id = 1,
        .sensor_data = &core_temp_sensor_data
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

// NMEA GPS sensor
struct nmea_sensor_data gps_sensor_data = {
        .uart = &uart0,
        .enable_pin = PIN_PTA18,
        .baudrate = 57600
};

struct sensor gps_sensor = {
        .type = &nmea_gps_sensor,
        .name = "gps",
        .sensor_id = 5,
        .sensor_data = &gps_sensor_data
};

// BMP085 pressure sensor
struct bmp085_sensor_data bmp085_sensor_data;

struct sensor bmp085_sensor = {
        .type = &bmp085_sensor_type,
        .name = "pressure",
        .sensor_id = 6,
        .sensor_data = &bmp085_sensor_data
};

// flow sensor
struct flow_sensor_data flow_sensor_data = {
        .count = 10
};

struct sensor flow_sensor = {
        .type = &flow_sensor_type,
        .name = "flow",
        .sensor_id = 7,
        .sensor_data = &flow_sensor_data
};

// battery voltage
struct batt_v_sensor_data battery_sensor_data = {
        .channel = ADC_PTD1,
        .enable_pin = GPIO_PTD5,
        .divider = 2,
};

struct sensor battery_voltage_sensor = {
        .type = &batt_v_sensor_type,
        .name = "battery voltage",
        .sensor_id = 8,
        .sensor_data = &battery_sensor_data
};

// color sensor
struct tcs_sensor_data tcs_sensor_data;

struct sensor tcs3472_sensor = {
        .type = &tcs3472_sensor_type,
        .name = "color",
        .sensor_id = 9,
        .sensor_data = &tcs_sensor_data
};

// sensor list
struct sensor *sensors[] = {
        &core_temp_sensor,
        &thermistor_sensor,
        //&thermistor2_sensor,
        //&conductivity_sensor,
        //&gps_sensor,
        //&bmp085_sensor,
        //&flow_sensor,
        //&battery_voltage_sensor,
        //&tcs3472_sensor,
        NULL
};

void
config_pins()
{
        // REMOTE_EN pin
        pin_mode(PIN_PTD3, PIN_MODE_MUX_GPIO);
        gpio_dir(PIN_PTD3, GPIO_OUTPUT);

        //nmea_init(&gps_sensor);

        // for bmp085
        i2c_init(I2C_RATE_100);
        pin_mode(PIN_PTA1, PIN_MODE_MUX_ALT2);
        pin_mode(PIN_PTA2, PIN_MODE_MUX_ALT2);

        // conductivity
        pin_mode(PIN_PTB2, PIN_MODE_MUX_GPIO); // EC_RANGE
        gpio_dir(PIN_PTB2, GPIO_OUTPUT); // overrides i2c pin muxing
        gpio_write(PIN_PTB2, GPIO_HIGH);
        cond_init();

        // LM19 (overrides i2c pin muxing)
        pin_mode(PIN_PTB3, PIN_MODE_MUX_ANALOG);

        batt_v_init(&battery_voltage_sensor);
        pin_mode(PIN_PTD1, PIN_MODE_MUX_ANALOG);
}
