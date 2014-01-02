#pragma once
#include <mchck.h>

// line received
extern void (*usb_console_line_recvd_cb)(const char *cmd, size_t len);

void usb_console_write_blocking(const uint8_t *buf, size_t len);

int usb_console_printf_blocking(char *fmt, ...);

void usb_console_tx(unsigned int len);

void usb_console_init();
