# STM32+Linux IoT Gateway

基于 STM32 + ESP8266 + Linux 的工业物联网网关，实现从传感器采集到云端存储的完整数据链路。

[![C](https://img.shields.io/badge/language-C11-blue)]()
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20STM32%20%7C%20ESP8266-green)]()
[![Build](https://img.shields.io/badge/build-CMake-orange)]()

## Architecture

```
STM32G431              ESP8266               Linux Gateway
┌────────────┐  UART   ┌──────────┐  WiFi    ┌───────────────────────────┐
│ ADC Sensor │────────→│ WiFi+MQTT│────────→ │ MQTT Subscribe (thread1)  │
│ Temp/Light │ 115200  │  Bridge  │  MQTT    │       ↓ cJSON parse       │
└────────────┘         └──────────┘          │ Ring Buffer (mutex+cond)  │
                                              │       ↓                   │
                                              │ DB Writer (thread2)       │→ SQLite
                                              │       ↓                   │
                                              │ TCP Server :8888 (thread3)│← Client
                                              │ Flask Dashboard :5000     │← Browser
                                              └───────────────────────────┘
```

## Features

- **3-thread architecture**: MQTT subscriber / DB writer / TCP server, thread-safe ring buffer with mutex + condition variable
- **MQTT protocol**: mosquitto C library, subscribe `sensor/data` topic, auto-reconnect
- **JSON parse**: cJSON lightweight parser, extract device/temp/humidity/pressure/light
- **SQLite storage**: auto-create table, timestamp insertion, SQL query support
- **TCP server**: port 8888, `get` command returns latest sensor data in JSON
- **Flask dashboard**: real-time web UI with auto-refresh, API `/api/data`
- **Signal handling**: graceful shutdown on Ctrl+C, all threads exit cleanly

## Tech Stack

| Layer | Technology |
|-------|-----------|
| MCU | STM32G431 / ADC / UART / HAL |
| WiFi Module | ESP8266 / MQTT (PubSubClient) |
| Gateway | Linux C11 / pthread / cJSON / SQLite |
| Web | Python Flask |
| Build | CMake / GCC |

## Quick Start

```bash
# Build
cd build && cmake .. && make

# Start MQTT broker
mosquitto -d

# Run gateway
./gateway

# Send test data (another terminal)
mosquitto_pub -t "sensor/data" -m '{"device":"stm32_g431","temp":25.6,"humidity":65.2,"pressure":1013.25,"light":512}'

# Query database
sqlite3 sensor.db "SELECT * FROM sensor;"

# TCP client test
nc localhost 8888
> get

# Web dashboard
cd ../web && python3 dashboard.py
# Open http://localhost:5000
```

## Project Structure

```
iot-gateway/
├── CMakeLists.txt              # Build script
├── include/
│   └── gateway.h               # Shared headers & data structures
├── src/
│   ├── main.c                  # Main thread: init + signal + cleanup
│   ├── mqtt_subscriber.c       # MQTT subscribe + cJSON parse
│   ├── ring_buffer.c           # Thread-safe ring buffer (mutex+cond)
│   ├── db_manager.c            # SQLite wrapper
│   ├── data_parser.c           # JSON → SensorData
│   └── tcp_server.c            # TCP server (:8888)
├── stm32/
│   └── sensor_uart.c           # STM32 ADC + UART → JSON
├── esp8266/
│   └── wifi_mqtt_bridge.ino    # ESP8266 WiFi + MQTT bridge
└── web/
    └── dashboard.py            # Flask web dashboard
```

## Thread Model

| Thread | Role | Sync |
|--------|------|------|
| main | init + signal handler + graceful shutdown | volatile flag |
| mqtt_thread | MQTT subscribe + JSON parse → push to ring buffer | mutex + cond_signal |
| db_writer | pop from ring buffer → SQLite insert | cond_timedwait (500ms) |
| tcp_server | accept client → return latest data | mutex lock latest |

## Author

**孙苏明** — Embedded Software Engineer

- GitHub: [@sunsuming-668](https://github.com/sunsuming-668)
