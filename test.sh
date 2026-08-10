#!/bin/bash
cd /home/sunsuming/iot-gateway/build

# 启动mosquitto（如果没有运行）
pgrep mosquitto > /dev/null || mosquitto -d
sleep 1

# 启动网关（后台）
./gateway &
GW_PID=$!
sleep 2

# 发送测试数据
echo "=== sending test data ==="
mosquitto_pub -t "sensor/data" -m '{"device":"stm32_g431","temp":25.6,"humidity":65.2,"pressure":1013.25,"light":512}'
sleep 1
mosquitto_pub -t "sensor/data" -m '{"device":"stm32_g431","temp":26.1,"humidity":63.8,"pressure":1012.80,"light":480}'
sleep 1
mosquitto_pub -t "sensor/data" -m '{"device":"stm32_g431","temp":24.8,"humidity":67.0,"pressure":1014.10,"light":620}'
sleep 1

# 查询数据库
echo ""
echo "=== database contents ==="
sqlite3 sensor.db "SELECT * FROM sensor;"

echo ""
echo "=== row count ==="
sqlite3 sensor.db "SELECT COUNT(*) FROM sensor;"

# 清理
kill $GW_PID 2>/dev/null
wait $GW_PID 2>/dev/null
echo ""
echo "=== test done ==="
