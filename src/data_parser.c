#include "gateway.h"

/* 解析JSON: {"device":"stm32","temp":25.6,"humidity":65.2,"pressure":1013.25,"light":512} */
int parse_sensor_json(const char *json_str, SensorData *data)
{
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return -1;

    memset(data, 0, sizeof(SensorData));

    cJSON *item;
    item = cJSON_GetObjectItem(root, "device");
    if (item && item->valuestring)
        strncpy(data->device, item->valuestring, sizeof(data->device) - 1);

    item = cJSON_GetObjectItem(root, "temp");
    if (item) data->temp = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "humidity");
    if (item) data->humidity = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "pressure");
    if (item) data->pressure = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "light");
    if (item) data->light = item->valueint;

    /* 生成本地时间戳 */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(data->timestamp, sizeof(data->timestamp), "%Y-%m-%d %H:%M:%S", t);

    cJSON_Delete(root);
    return 0;
}
