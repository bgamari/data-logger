#pragma once
#include <mchck.h>
#include "sensor.h"

enum nmea_flavor {
        NMEA_GENERIC,
        NMEA_MTK
};

struct nmea_sensor_data {
        struct uart_ctx *uart;
        enum pin_id enable_pin;
        unsigned int baudrate;
        enum nmea_flavor flavor;

        /* private */
        struct uart_trans_ctx trans;
        char buffer[128];
        accum values[4];
};

void nmea_init(struct sensor *sensor);

extern struct sensor_type nmea_gps_sensor;
