#include <stdbool.h>

#include "usb_console.h"
#include "data-logger.desc.h"

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
usb_console_write_blocking(const uint8_t *buf, size_t len)
{
        size_t remaining = len;
        while (true) {
                size_t written = cdc_write((const uint8_t*) buf, remaining, &cdc);
                remaining -= written;
                buf += written;
                if (remaining > 0)
                        __asm__("wfi");
                else
                        break;
        }
}

static size_t
out_file_write(const uint8_t *buf, size_t len, void *ops_data)
{
        usb_console_write_blocking(buf, len);
        return len;
}

struct _stdio_file_ops out_file_ops = {
        .init = NULL,
        .write = out_file_write
};

FILE out_file = {
        .ops = &out_file_ops,
};

int
usb_console_printf_blocking(char *fmt, ...)
{
        va_list args;
        int ret;

        va_start(args, fmt);
        ret = vfprintf(&out_file, fmt, args);
        va_end(args);
        return ret;
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
