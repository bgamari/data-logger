#pragma once
#include <mchck.h>

typedef void (*flush_cb)(void *cbdata);

// line received
extern void (*usb_console_line_recvd_cb)(const char *cmd, size_t len);

int usb_console_printf(const char *fmt, ...);

size_t usb_console_write(const char *buf, size_t len);

// flush transmit buffer
void usb_console_flush(flush_cb cb, void *cbdata);

void usb_console_reset();

void usb_console_init();

void usb_console_power_down();
