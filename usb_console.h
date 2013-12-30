#pragma once
#include <mchck.h>

// poke data into this array then call usb_console_tx
extern char tx_buffer[255];

// line received
extern void (*usb_console_line_recvd_cb)(char *cmd, size_t len);

void usb_console_tx(unsigned int len);

void usb_console_init();
