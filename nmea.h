#pragma once
#include <mchck.h>
#include "sensor.h"

struct nmea_sensor_data {
        struct uart_ctx *uart;
        enum pin_id enable_pin;
        unsigned int baudrate;

        /* private */
        struct uart_trans_ctx trans;
        char buffer[255];
};

void nmea_init(struct sensor *sensor);

extern struct sensor_type nmea_gps_sensor;
