#ifndef GATEWAY_H
#define GATEWAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <mosquitto.h>
#include <cjson/cJSON.h>

/* ====== 配置 ====== */
#define MQTT_BROKER_HOST    "127.0.0.1"
#define MQTT_BROKER_PORT    1883
#define MQTT_TOPIC          "sensor/data"
#define MQTT_CLIENT_ID      "iot_gateway"

#define DB_PATH             "sensor.db"

#define TCP_SERVER_PORT     8888
#define TCP_MAX_CLIENTS     5
#define RING_BUF_SIZE       128

/* ====== 传感器数据 ====== */
typedef struct {
    char    device[32];
    float   temp;
    float   humidity;
    float   pressure;
    int     light;
    char    timestamp[32];
} SensorData;

/* ====== 环形缓冲区（线程安全） ====== */
typedef struct {
    SensorData      buf[RING_BUF_SIZE];
    int             head;
    int             tail;
    int             count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} RingBuffer;

/* ====== 全局上下文 ====== */
typedef struct {
    RingBuffer      ring;
    SensorData      latest;
    pthread_mutex_t latest_mutex;
    volatile int    running;
} GatewayCtx;

/* ====== 函数声明 ====== */
void ring_init(RingBuffer *rb);
void ring_push(RingBuffer *rb, const SensorData *data);
int  ring_pop(RingBuffer *rb, SensorData *data, int timeout_ms);

int  db_init(const char *path);
int  db_insert(const SensorData *data);
void db_close(void);

int  parse_sensor_json(const char *json_str, SensorData *data);

void *mqtt_thread_func(void *arg);
void *tcp_server_thread_func(void *arg);

#endif
