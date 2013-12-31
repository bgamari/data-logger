#include "config.h"

#include "thermistor.h"
#include "temperature.h"
#include "conductivity.h"

// on-board temperature
struct sensor temperature_sensor = {
        .sample = &adc_sensor_sample,
        .name = "temperature",
        .sensor_id = 1,
        .sensor_data = &temperature_sensor_data
};

struct thermistor_map_data thermistor_map_data = {
        .beta = 3700,
        .t0 = 298,
        .r0 = 10e3,
        .r1 = 10e3
};

// thermistor #1
struct adc_sensor_data thermistor_adc_sensor_data = {
        .channel = ADC_PTD6,
        .map = thermistor_map,
        .map_data = &thermistor_map_data,
};
        
struct sensor thermistor_sensor = {
        .sample = &adc_sensor_sample,
        .sensor_id = 2,
        .sensor_data = &thermistor_adc_sensor_data,
};

// thermistor #2
struct adc_sensor_data thermistor2_adc_sensor_data = {
        .channel = ADC_PTD5,
        .map = thermistor_map,
        .map_data = &thermistor_map_data,
};

struct sensor thermistor2_sensor = {
        .sample = &adc_sensor_sample,
        .sensor_id = 3,
        .sensor_data = &thermistor2_adc_sensor_data,
};

// conductivity sensor
struct cond_sensor_data cond_sensor_data;

struct sensor conductivity_sensor = {
        .sample = &cond_sensor_sample,
        .sensor_id = 4,
        .sensor_data = &cond_sensor_data,
};

struct sensor *sensors[] = {
        &temperature_sensor,
        &thermistor_sensor,
        &thermistor2_sensor,
        &conductivity_sensor,
        NULL
};
