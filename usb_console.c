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

// ACM transmit
#define TX_BUF_SIZE 64
static char tx_buffer[TX_BUF_SIZE];
static volatile unsigned int tx_head = 0; // where data is placed
static volatile size_t tx_tail = 0; // where data is read out

static flush_cb cur_flush_cb = NULL;
static void *flush_cbdata = NULL;

static size_t
out_file_write(const uint8_t *buf, size_t len, void *ops_data)
{
        crit_enter();
        if (len + tx_head > TX_BUF_SIZE)
                len = TX_BUF_SIZE - tx_head;
        memcpy(&tx_buffer[tx_head], buf, len);
        tx_head += len;
        crit_exit();
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
usb_console_printf(const char *fmt, ...)
{
        va_list args;
        int ret;

        va_start(args, fmt);
        ret = vfprintf(&out_file, fmt, args);
        va_end(args);
        return ret;
}

static void
do_flush(size_t sent)
{
        crit_enter();
        if (tx_tail < tx_head) {
                size_t written = cdc_write((const uint8_t *) &tx_buffer[tx_tail],
                                           tx_head - tx_tail, &cdc);
                tx_tail += written;
                crit_exit();
        } else {
                tx_head = 0;
                flush_cb cb = cur_flush_cb;
                cur_flush_cb = NULL;
                crit_exit();
                cb(flush_cbdata);
        }
}

void
usb_console_flush(flush_cb cb, void *cbdata)
{
        crit_enter();
        cur_flush_cb = cb;
        flush_cbdata = cbdata;
        tx_tail = 0;
        do_flush(0);
        crit_exit();
}

static void
data_sent_cb(size_t sent)
{
        do_flush(sent);
}

void
usb_console_reset()
{
        crit_enter();
        rx_tail = 0;
        tx_head = 0;
        tx_tail = 0;
        crit_exit();
}

void
init_vcdc(int config)
{
        cdc_init(new_data, data_sent_cb, &cdc);
        cdc_set_stdout(&cdc);
}

void
usb_console_init()
{
        usb_console_reset();
        usb_init(&cdc_device);
}
