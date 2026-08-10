#include "gateway.h"

void ring_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    pthread_mutex_init(&rb->mutex, NULL);
    pthread_cond_init(&rb->not_empty, NULL);
    pthread_cond_init(&rb->not_full, NULL);
}

void ring_push(RingBuffer *rb, const SensorData *data)
{
    pthread_mutex_lock(&rb->mutex);
    while (rb->count >= RING_BUF_SIZE) {
        pthread_cond_wait(&rb->not_full, &rb->mutex);
    }
    rb->buf[rb->head] = *data;
    rb->head = (rb->head + 1) % RING_BUF_SIZE;
    rb->count++;
    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
}

int ring_pop(RingBuffer *rb, SensorData *data, int timeout_ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&rb->mutex);
    while (rb->count == 0) {
        int rc = pthread_cond_timedwait(&rb->not_empty, &rb->mutex, &ts);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&rb->mutex);
            return -1;
        }
    }
    *data = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUF_SIZE;
    rb->count--;
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    return 0;
}
