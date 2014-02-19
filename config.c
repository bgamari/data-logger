#include "config.h"

#include "thermistor.h"
#include "temperature.h"
#include "conductivity.h"
#include "acquire.h"

// on-board temperature
struct sensor temperature_sensor = {
        .type = &adc_sensor,
        .name = "onboard-temperature",
        .unit = "Kelvin",
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
        .type = &adc_sensor,
        .name = "external temperature",
        .unit = "Celcius",
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
        .unit = "Kelvin",
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
        .unit = "arbitrary",
        .sensor_id = 4,
        .sensor_data = &cond_sensor_data,
};

struct sensor *sensors[] = {
        &temperature_sensor,
        &thermistor_sensor,
        //&thermistor2_sensor,
        &conductivity_sensor,
        NULL
};

void
config_pins()
{
        pin_mode(PIN_PTB0, PIN_MODE_MUX_ANALOG);
        pin_mode(PIN_PTD5, PIN_MODE_MUX_ANALOG);
        pin_mode(PIN_PTA1, PIN_MODE_MUX_ALT3 | PIN_MODE_PULLUP);
        //PORTA.pcr[1].irqc = PCR_IRQC_INT_FALLING;
}
