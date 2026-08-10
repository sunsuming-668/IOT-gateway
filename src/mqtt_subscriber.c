#include "gateway.h"

extern GatewayCtx g_ctx;

/* MQTT消息回调 */
static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg)
{
    (void)mosq; (void)userdata;

    if (!msg->payload || msg->payloadlen == 0) return;

    SensorData data;
    if (parse_sensor_json((const char *)msg->payload, &data) == 0) {
        /* 推入环形缓冲区 -> DB线程消费 */
        ring_push(&g_ctx.ring, &data);

        /* 更新latest -> TCP线程读取 */
        pthread_mutex_lock(&g_ctx.latest_mutex);
        g_ctx.latest = data;
        pthread_mutex_unlock(&g_ctx.latest_mutex);

        printf("[MQTT] %s  T=%.1f H=%.1f P=%.1f L=%d\n",
               data.device, data.temp, data.humidity,
               data.pressure, data.light);
    } else {
        fprintf(stderr, "[MQTT] parse failed: %s\n", (char *)msg->payload);
    }
}

void *mqtt_thread_func(void *arg)
{
    (void)arg;

    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new(MQTT_CLIENT_ID, true, NULL);
    if (!mosq) {
        fprintf(stderr, "[MQTT] mosquitto_new failed\n");
        return NULL;
    }

    mosquitto_message_callback_set(mosq, on_message);

    int rc = mosquitto_connect(mosq, MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[MQTT] connect failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    mosquitto_subscribe(mosq, NULL, MQTT_TOPIC, 0);
    printf("[MQTT] subscribed to %s @ %s:%d\n",
           MQTT_TOPIC, MQTT_BROKER_HOST, MQTT_BROKER_PORT);

    /* 事件循环，阻塞直到running=0 */
    while (g_ctx.running) {
        rc = mosquitto_loop(mosq, 1000, 1);
        if (rc != MOSQ_ERR_SUCCESS) {
            printf("[MQTT] reconnecting...\n");
            sleep(2);
            mosquitto_reconnect(mosq);
        }
    }

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    printf("[MQTT] thread exit\n");
    return NULL;
}
