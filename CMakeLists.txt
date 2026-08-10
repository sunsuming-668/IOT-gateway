cmake_minimum_required(VERSION 3.10)
project(iot-gateway C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pthread")

# 源文件
set(SOURCES
    src/main.c
    src/ring_buffer.c
    src/data_parser.c
    src/db_manager.c
    src/mqtt_subscriber.c
    src/tcp_server.c
)

add_executable(gateway ${SOURCES})

# 头文件路径
target_include_directories(gateway PRIVATE include)

# 链接库
target_link_libraries(gateway
    mosquitto
    sqlite3
    cjson
    pthread
)
