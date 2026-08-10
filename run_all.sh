#!/bin/bash
set -e
DIR="/home/sunsuming/iot-gateway"
cd "$DIR"

# 启动mosquitto
pgrep mosquitto > /dev/null || mosquitto -d
sleep 1

# 编译
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -1
make -j4 2>&1 | tail -3
cd ..

# 删除旧数据库
rm -f build/sensor.db

# 启动C网关
./build/gateway &
GW_PID=$!
sleep 2

# 测试数据写入临时文件
cat > /tmp/test_msg1.txt << 'EOF'
{"device":"stm32_g431","temp":25.6,"humidity":65.2,"pressure":1013.25,"light":512}
EOF
cat > /tmp/test_msg2.txt << 'EOF'
{"device":"stm32_g431","temp":26.1,"humidity":63.8,"pressure":1012.80,"light":480}
EOF
cat > /tmp/test_msg3.txt << 'EOF'
{"device":"stm32_g431","temp":24.8,"humidity":67.0,"pressure":1014.10,"light":620}
EOF

mosquitto_pub -t "sensor/data" -f /tmp/test_msg1.txt
sleep 0.5
mosquitto_pub -t "sensor/data" -f /tmp/test_msg2.txt
sleep 0.5
mosquitto_pub -t "sensor/data" -f /tmp/test_msg3.txt
sleep 1

echo "=== DB contents ==="
sqlite3 build/sensor.db "SELECT * FROM sensor;"
echo ""

# 启动Flask（前台）
echo "=== starting Flask at http://localhost:5000 ==="
cd web
python3 dashboard.py
