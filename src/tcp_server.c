#include "gateway.h"

extern GatewayCtx g_ctx;

static void handle_client(int client_fd)
{
    char buf[256];
    SensorData snapshot;

    /* 发送欢迎消息 */
    const char *welcome = "IoT Gateway OK. Type 'get' for latest data, 'quit' to exit.\n";
    send(client_fd, welcome, strlen(welcome), 0);

    while (g_ctx.running) {
        memset(buf, 0, sizeof(buf));
        int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;

        /* 去掉换行 */
        buf[strcspn(buf, "\r\n")] = 0;

        if (strcmp(buf, "get") == 0) {
            pthread_mutex_lock(&g_ctx.latest_mutex);
            snapshot = g_ctx.latest;
            pthread_mutex_unlock(&g_ctx.latest_mutex);

            char resp[256];
            snprintf(resp, sizeof(resp),
                "{\"device\":\"%s\",\"temp\":%.2f,\"humidity\":%.2f,"
                "\"pressure\":%.2f,\"light\":%d,\"ts\":\"%s\"}\n",
                snapshot.device, snapshot.temp, snapshot.humidity,
                snapshot.pressure, snapshot.light, snapshot.timestamp);
            send(client_fd, resp, strlen(resp), 0);
        } else if (strcmp(buf, "quit") == 0) {
            break;
        } else {
            const char *err = "unknown cmd\n";
            send(client_fd, err, strlen(err), 0);
        }
    }
    close(client_fd);
}

void *tcp_server_thread_func(void *arg)
{
    (void)arg;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[TCP] socket");
        return NULL;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(TCP_SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[TCP] bind");
        close(server_fd);
        return NULL;
    }

    listen(server_fd, TCP_MAX_CLIENTS);
    printf("[TCP] listening on port %d\n", TCP_SERVER_PORT);

    while (g_ctx.running) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (client_fd < 0) continue;

        printf("[TCP] client connected: %s:%d\n",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        handle_client(client_fd);
        printf("[TCP] client disconnected\n");
    }

    close(server_fd);
    printf("[TCP] thread exit\n");
    return NULL;
}
