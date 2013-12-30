#include "temp-gather.desc.h"

static struct cdc_ctx cdc;

// ACM device receive buffer
static char rx_buffer[32];
static unsigned int rx_tail = 0;
void (*usb_console_line_recvd_cb)(const char *cmd, size_t len) = NULL;

static void
new_data(uint8_t *data, size_t len)
{
        for (unsigned int i=0; i<len; i++) {
                if (data[i] == '\n' || data[i] == '\r') {
                        rx_buffer[rx_tail] = 0;
                        if (usb_console_line_recvd_cb)
                                usb_console_line_recvd_cb(rx_buffer, rx_tail);
                        rx_tail = 0;
                } else if (data[i] == '\b') {
                        // backspace
                } else {
                        rx_buffer[rx_tail] = data[i];
                        rx_tail = (rx_tail + 1) % sizeof(rx_buffer);
                }
        }
        
        cdc_read_more(&cdc);
}

void
init_vcdc(int config)
{
        cdc_init(new_data, NULL, &cdc);
        cdc_set_stdout(&cdc);
}

void
usb_console_init()
{
        usb_init(&cdc_device);
}