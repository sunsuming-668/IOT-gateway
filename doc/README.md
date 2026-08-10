# 基于STM32+Linux的工业物联网网关

## 项目概述
完整的嵌入式物联网数据链路：STM32传感器采集 → ESP8266 WiFi透传 → Linux网关（MQTT订阅/C多线程/SQLite存储/TCP服务）

## 系统架构

```
STM32G431                ESP8266              Linux Gateway (WSL2/Ubuntu)
┌──────────┐  UART 115200  ┌──────┐  WiFi+MQTT  ┌─────────────────────────┐
│ ADC采集   │──────────────→│ 透传  │────────────→│ MQTT Broker (mosquitto) │
│ 温度/光照 │              │ 桥接  │             │          ↓              │
└──────────┘              └──────┘             │ MQTT线程 → cJSON解析     │
                                               │          ↓              │
                                               │ 环形缓冲区(mutex+cond)  │
                                               │          ↓              │
                                               │ DB线程 → SQLite入库     │
                                               │          ↓              │
                                               │ TCP Server(:8888)       │
                                               └─────────────────────────┘
```

## 技术栈

| 层级 | 技术 |
|------|------|
| 硬件端 | STM32G431 / ADC / UART / HAL库 |
| 通信模块 | ESP8266 / WiFi / MQTT (mosquitto) |
| 网关 | Linux / C11 / pthread / cJSON / SQLite |
| 构建 | CMake / GCC |

## 文件结构

```
iot-gateway/
├── CMakeLists.txt
├── include/gateway.h           ← 公共头文件
├── src/
│   ├── main.c                  ← 主线程(初始化+信号处理)
│   ├── mqtt_subscriber.c       ← MQTT订阅线程
│   ├── ring_buffer.c           ← 线程安全环形缓冲区
│   ├── db_manager.c            ← SQLite封装
│   ├── data_parser.c           ← cJSON JSON解析
│   └── tcp_server.c            ← TCP服务器线程
├── stm32/sensor_uart.c         ← STM32传感器采集+UART发送
├── esp8266/wifi_mqtt_bridge.ino ← ESP8266 WiFi+MQTT桥接
├── web/                        ← Flask仪表盘(待开发)
└── test.sh                     ← 测试脚本
```

## 编译与运行

```bash
# WSL2中
cd ~/iot-gateway/build
cmake ..
make

# 启动mosquitto
mosquitto -d

# 运行网关
./gateway

# 测试（另一个终端）
mosquitto_pub -t "sensor/data" -m '{"device":"stm32_g431","temp":25.6,"humidity":65.2,"pressure":1013.25,"light":512}'

# 查询数据库
sqlite3 sensor.db "SELECT * FROM sensor;"

# TCP客户端测试
nc localhost 8888
get
```

## 线程模型

| 线程 | 职责 | 同步机制 |
|------|------|----------|
| main | 初始化+信号处理+优雅退出 | volatile running标志 |
| mqtt_thread | MQTT订阅+JSON解析 | 无锁写入环形缓冲区 |
| db_writer | 从缓冲区取数据→写SQLite | mutex+cond |
| tcp_server | 接受客户端连接+返回最新数据 | mutex保护latest |
