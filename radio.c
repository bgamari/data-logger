#include <mchck.h>

#include "sensor.h"

static bool radio_enabled = false;
static struct sensor_listener radio_listener;
static unsigned int dropped_samples = 0;

struct nrf_sample {
        uint32_t time;
        uint8_t sensor_id;
        accum value;
};

#define NBUFFERS 2
#define BUFFER_LEN 16

struct nrf_buffer {
        struct nrf_sample buffer[BUFFER_LEN];
        unsigned int head;
        struct nrf_buffer *next;
};
static struct nrf_buffer radio_buffers[NBUFFERS];
static struct nrf_buffer *fill_queue;
static struct nrf_buffer *send_queue;

struct nrf_addr_t dest_addr = {
	.value =  0xf0f0f0f0e1ll,
	.size = 5
};

void
radio_init()
{
        for (unsigned int i=0; i<NBUFFERS; i++)
                radio_buffers[i].next = &radio_buffers[i+1];
        radio_buffers[NBUFFERS-1].next = NULL;
        fill_queue = &radio_buffers[0];
                        
        nrf_init();
	nrf_set_autoretransmit(3, 5); // 1000us, 5x
	nrf_set_channel(16);
	nrf_set_power_datarate(NRF_TX_POWER_0DBM, NRF_DATA_RATE_1MBPS);
	nrf_enable_dynamic_payload();
	nrf_enable_powersave();
}

static void radio_send();

static void
radio_sample_sent(void *payload, uint8_t bytes_sent)
{
        crit_enter();
        struct nrf_buffer *sent = send_queue;
        send_queue = send_queue->next;

        // find tail of fill queue
        struct nrf_buffer *tail = fill_queue;
        while (tail->next != NULL)
                tail = tail->next;

        // place buffer on end of fill queue
        sent->next = NULL;
        sent->head = 0;
        tail->next = sent;

        // send next
        radio_send();
        crit_exit();
}

// send the buffer at the head of the send queue
static void
radio_send()
{
        if (send_queue) {
                nrf_send(&dest_addr, send_queue->buffer,
                         sizeof(struct nrf_sample) * send_queue->head,
                         radio_sample_sent);
        }
}
        
// send the buffer at the head of the fill queue
static void
radio_finish_buffer()
{
        crit_enter();
        struct nrf_buffer *done = fill_queue;
        fill_queue = fill_queue->next;

        if (send_queue) {
                // find tail of send queue
                struct nrf_buffer *tail = send_queue;
                while (tail->next != NULL)
                        tail = tail->next;

                done->next = NULL;
                tail->next = done;
        } else {
                send_queue = done;
                radio_send();
        }
        crit_exit();
}

static void
radio_new_sample(struct sensor *sensor, accum value, void *cbdata)
{
        crit_enter();
        if (!fill_queue) {
                dropped_samples++;
                crit_exit();
                return;
        }

        fill_queue->buffer[fill_queue->head] = (struct nrf_sample) {
                .time = rtc_get_time(),
                .sensor_id = sensor->sensor_id,
                .value = value
        };
        fill_queue->head++;

        if (fill_queue->head == BUFFER_LEN) {
                radio_finish_buffer();
        }
        crit_exit();
}

void
radio_enable()
{
        if (radio_enabled)
                return;

        sensor_listen(&radio_listener, radio_new_sample, NULL);
        radio_enabled = true;
}

void
radio_disable()
{
        radio_enabled = false;
}

bool
radio_get_enabled()
{
        return radio_enabled;
}
