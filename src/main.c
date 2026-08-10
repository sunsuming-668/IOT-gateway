#include "gateway.h"

/* 全局上下文 */
GatewayCtx g_ctx;

/* 信号处理 -> 优雅退出 */
static void sig_handler(int sig)
{
    (void)sig;
    printf("\n[MAIN] shutting down...\n");
    g_ctx.running = 0;
}

/* DB消费者线程：从环形缓冲区取数据写入SQLite */
static void *db_writer_thread(void *arg)
{
    (void)arg;
    SensorData data;

    while (g_ctx.running) {
        if (ring_pop(&g_ctx.ring, &data, 500) == 0) {
            db_insert(&data);
        }
    }
    printf("[DB] thread exit\n");
    return NULL;
}

int main(void)
{
    printf("========================================\n");
    printf("  IoT Gateway v1.0\n");
    printf("  MQTT -> SQLite -> TCP Server\n");
    printf("========================================\n\n");

    /* 初始化全局状态 */
    g_ctx.running = 1;
    memset(&g_ctx.latest, 0, sizeof(SensorData));
    ring_init(&g_ctx.ring);
    pthread_mutex_init(&g_ctx.latest_mutex, NULL);

    /* 注册信号 */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* 初始化数据库 */
    if (db_init(DB_PATH) != 0) {
        fprintf(stderr, "[MAIN] DB init failed\n");
        return 1;
    }

    /* 启动三个线程 */
    pthread_t mqtt_tid, db_tid, tcp_tid;

    pthread_create(&mqtt_tid, NULL, mqtt_thread_func, NULL);
    pthread_create(&db_tid,   NULL, db_writer_thread,  NULL);
    pthread_create(&tcp_tid,  NULL, tcp_server_thread_func, NULL);

    printf("[MAIN] all threads started. Press Ctrl+C to stop.\n\n");

    /* 等待退出信号 */
    while (g_ctx.running) {
        sleep(1);
    }

    /* 等待线程结束 */
    pthread_join(mqtt_tid, NULL);
    pthread_join(db_tid,   NULL);
    /* TCP accept是阻塞的，shutdown让它退出 */
    pthread_cancel(tcp_tid);
    pthread_join(tcp_tid,  NULL);

    /* 清理 */
    db_close();
    pthread_mutex_destroy(&g_ctx.latest_mutex);
    printf("[MAIN] bye.\n");
    return 0;
}
