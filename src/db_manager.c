#include "gateway.h"

static sqlite3 *g_db = NULL;

int db_init(const char *path)
{
    int rc = sqlite3_open(path, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] open failed: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS sensor ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  device    TEXT,"
        "  temp      REAL,"
        "  humidity  REAL,"
        "  pressure  REAL,"
        "  light     INTEGER,"
        "  ts        TEXT"
        ");";

    char *err = NULL;
    rc = sqlite3_exec(g_db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] create table: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    printf("[DB] initialized: %s\n", path);
    return 0;
}

int db_insert(const SensorData *data)
{
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO sensor (device, temp, humidity, pressure, light, ts) "
        "VALUES ('%s', %.2f, %.2f, %.2f, %d, '%s');",
        data->device, data->temp, data->humidity,
        data->pressure, data->light, data->timestamp);

    char *err = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] insert: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

void db_close(void)
{
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}
