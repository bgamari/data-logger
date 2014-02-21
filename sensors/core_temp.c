#include "core_temp.h"

static accum
core_temp_map(uint16_t codepoint, void *map_data)
{
        unsigned accum voltage = adc_as_voltage(codepoint);
        accum volt_diff = voltage - 0.719k;
        accum temp_diff = volt_diff * (1000K / 1.715K);
        accum temp_deg = 25k - temp_diff;
        return temp_deg;
}

static struct measurable core_temp_measurables[] = {
        {.id = 0, .name = "temperature", .unit = "Kelvin"}
};

struct sensor_type core_temp_sensor_type = {
        .sample_fn = &adc_sensor_sample,
        .no_stop = true,
        .n_measurables = 1,
        .measurables = core_temp_measurables
};

struct adc_sensor_data core_temp_sensor_data = {
        .channel = ADC_TEMP,
        .mode = ADC_AVG_32 | ADC_MODE_POWER_LOW | ADC_MODE_SAMPLE_LONG,
        .map = core_temp_map
};
