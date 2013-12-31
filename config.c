#include "config.h"

#include "thermistor.h"
#include "temperature.h"

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

#if 0
struct sensor conductivity_sensor = {
        .sample = &conductivity_sensor_sample,
        .sensor_id = 3,
        .sensor_data = &conductivity_sensor_data,
};
#endif

struct sensor *sensors[] = {
        &temperature_sensor,
        &thermistor_sensor,
        &thermistor2_sensor,
        //&conductivity_sensor,
        NULL
};
