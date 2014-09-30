#include "config.h"
#include "acquire.h"

#include "sensors/thermistor.h"
#include "sensors/core_temp.h"
#include "sensors/lm19.h"
#include "sensors/conductivity_gpio.h"
#include "sensors/battery_voltage.h"
#include "sensors/tcs3472_sensor.h"
#include "sensors/tmp100.h"

/* Riffle 4.0 configuration */

// core temperature
struct sensor core_temp_sensor = {
        .type = &core_temp_sensor_type,
        .name = "core temperature",
        .sensor_id = 1,
        .sensor_data = &core_temp_sensor_data
};

// conductivity sensor
struct cond_gpio_sensor_data cond_sensor_data = {
        .pin_a = PIN_PTD4,
        .transitions = 10,
};

struct sensor conductivity_sensor = {
        .type = &cond_gpio_sensor_type,
        .name = "conductivity",
        .sensor_id = 4,
        .sensor_data = &cond_sensor_data,
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
struct tcs_sensor_data tcs_sensor_data = {
        .tcs = {.address = 0x29},
        .gain = TCS_GAIN_60,
        .int_time = 0x00,
};

struct sensor tcs3472_sensor = {
        .type = &tcs3472_sensor_type,
        .name = "color",
        .sensor_id = 9,
        .sensor_data = &tcs_sensor_data
};

// tmp100
struct tmp100_sensor_data tmp100_sensor_data = {
        .address = 0x4f
};

struct sensor tmp100_sensor = {
        .type = &tmp100_sensor_type,
        .name = "temperature",
        .sensor_id = 10,
        .sensor_data = &tmp100_sensor_data
};

// sensor list
struct sensor *sensors[] = {
        &core_temp_sensor,
        &conductivity_sensor,
        &battery_voltage_sensor,
        &tcs3472_sensor,
        &tmp100_sensor,
        NULL
};

void
config_pins()
{
        // REMOTE_EN pin
        pin_mode(PIN_PTD3, PIN_MODE_MUX_GPIO);
        gpio_dir(PIN_PTD3, GPIO_OUTPUT);

        // UART
        pin_mode(PIN_PTA1, PIN_MODE_MUX_ALT2);
        pin_mode(PIN_PTA2, PIN_MODE_MUX_ALT2);

        // for I2C devices
        i2c_init(I2C_RATE_100);
        PORTB.pcr[pin_physpin_from_pin(PIN_PTB0)].raw = ((struct PCR_t) {.mux=2,.ode=1}).raw;
        PORTB.pcr[pin_physpin_from_pin(PIN_PTB1)].raw = ((struct PCR_t) {.mux=2,.ode=1}).raw;

        // conductivity
        pin_mode(PIN_PTB2, PIN_MODE_MUX_GPIO); // EC_RANGE
        gpio_dir(PIN_PTB2, GPIO_OUTPUT); // overrides i2c pin muxing
        gpio_write(PIN_PTB2, GPIO_HIGH);
        pin_mode(PIN_PTD4, PIN_MODE_MUX_GPIO); // EC_A
        gpio_dir(PIN_PTD4, GPIO_OUTPUT);
        gpio_write(PIN_PTD4, GPIO_HIGH);
        pin_mode(PIN_PTB3, PIN_MODE_MUX_GPIO); // EC_B
        gpio_dir(PIN_PTB3, GPIO_OUTPUT);
        gpio_write(PIN_PTB3, GPIO_HIGH);
        pin_mode(PIN_PTC2, PIN_MODE_MUX_ANALOG); // EC_OUT
        cond_gpio_init(&cond_sensor_data);

        // battery voltage
        batt_v_init(&battery_voltage_sensor);
        pin_mode(PIN_PTD1, PIN_MODE_MUX_ANALOG);

        tmp100_hack();
}
