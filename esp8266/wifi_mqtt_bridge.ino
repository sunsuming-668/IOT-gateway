/**
 * ESP8266 WiFi + MQTT 桥接模块
 * -----------------------------------------------
 * 硬件：ESP8266-01S / NodeMCU
 * 功能：UART接收STM32数据 → WiFi → MQTT发布
 * Arduino IDE + ESP8266开发板包
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* ====== WiFi配置 ====== */
const char* WIFI_SSID     = "你的WiFi名";
const char* WIFI_PASSWORD = "你的WiFi密码";

/* ====== MQTT配置 ====== */
const char* MQTT_BROKER   = "192.168.1.100";  /* Linux网关IP */
const int   MQTT_PORT     = 1883;
const char* MQTT_TOPIC    = "sensor/data";
const char* MQTT_CLIENT   = "esp8266_bridge";

/* ====== 串口配置 ====== */
#define SERIAL_BAUD     115200
#define UART_RX_BUF     256

WiFiClient espClient;
PubSubClient mqtt(espClient);
char uartBuf[UART_RX_BUF];
int uartIdx = 0;

/* WiFi连接 */
void setup_wifi()
{
    delay(10);
    Serial.printf("\n[WIFI] connecting to %s", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (++retry > 40) {
            Serial.println("\n[WIFI] failed, retrying...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            retry = 0;
        }
    }

    Serial.printf("\n[WIFI] connected, IP: %s\n", WiFi.localIP().toString().c_str());
}

/* MQTT重连 */
void mqtt_reconnect()
{
    while (!mqtt.connected()) {
        Serial.print("[MQTT] connecting...");
        if (mqtt.connect(MQTT_CLIENT)) {
            Serial.println(" OK");
        } else {
            Serial.printf(" failed, rc=%d, retry in 3s\n", mqtt.state());
            delay(3000);
        }
    }
}

/* 处理UART收到的一行JSON */
void handle_uart_line(const char* line)
{
    /* 验证是合法JSON */
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) {
        Serial.printf("[UART] invalid JSON: %s\n", line);
        return;
    }

    /* 直接转发到MQTT（STM32已经格式化好了） */
    if (mqtt.connected()) {
        mqtt.publish(MQTT_TOPIC, line);
        Serial.printf("[MQTT] published: %s\n", line);
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    Serial.println("\n=== ESP8266 MQTT Bridge ===");

    setup_wifi();
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt_reconnect();
}

void loop()
{
    /* 保持MQTT连接 */
    if (!mqtt.connected()) {
        mqtt_reconnect();
    }
    mqtt.loop();

    /* 读取UART数据（按行分割） */
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            uartBuf[uartIdx] = '\0';
            if (uartIdx > 0) {
                handle_uart_line(uartBuf);
            }
            uartIdx = 0;
        } else if (uartIdx < UART_RX_BUF - 1) {
            uartBuf[uartIdx++] = c;
        }
    }
}
