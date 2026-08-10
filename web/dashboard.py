#!/usr/bin/env python3
"""
IoT Gateway Web Dashboard
Flash + SQLite 读取传感器数据，实时展示
"""

from flask import Flask, jsonify, render_template_string
import sqlite3
import os

app = Flask(__name__)
DB_PATH = os.path.join(os.path.dirname(__file__), '..', 'build', 'sensor.db')

HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Gateway Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', sans-serif; background: #0f1923; color: #e0e0e0; }
        .header { background: #1a2733; padding: 16px 24px; border-bottom: 2px solid #00bcd4; }
        .header h1 { font-size: 20px; color: #00bcd4; }
        .header span { font-size: 13px; color: #78909c; margin-left: 12px; }
        .cards { display: flex; gap: 16px; padding: 20px 24px; flex-wrap: wrap; }
        .card { background: #1a2733; border-radius: 8px; padding: 20px; flex: 1; min-width: 180px; border-left: 3px solid #00bcd4; }
        .card .label { font-size: 12px; color: #78909c; text-transform: uppercase; }
        .card .value { font-size: 32px; font-weight: bold; color: #fff; margin-top: 4px; }
        .card .unit { font-size: 14px; color: #78909c; }
        .table-wrap { padding: 0 24px 24px; }
        table { width: 100%; border-collapse: collapse; background: #1a2733; border-radius: 8px; overflow: hidden; }
        th { background: #263545; padding: 10px 16px; text-align: left; font-size: 13px; color: #00bcd4; }
        td { padding: 8px 16px; border-bottom: 1px solid #263545; font-size: 13px; }
        tr:hover td { background: #1e3040; }
        .status { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: #4caf50; margin-right: 6px; }
        .footer { text-align: center; padding: 16px; color: #546e7a; font-size: 12px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>IoT Gateway Dashboard
            <span><span class="status"></span>Live</span>
        </h1>
    </div>

    <div class="cards" id="cards">
        <div class="card"><div class="label">Temperature</div><div class="value" id="temp">--</div><div class="unit">°C</div></div>
        <div class="card"><div class="label">Humidity</div><div class="value" id="humi">--</div><div class="unit">%</div></div>
        <div class="card"><div class="label">Pressure</div><div class="value" id="pres">--</div><div class="unit">hPa</div></div>
        <div class="card"><div class="label">Light</div><div class="value" id="light">--</div><div class="unit">ADC</div></div>
        <div class="card"><div class="label">Records</div><div class="value" id="count">--</div><div class="unit">rows</div></div>
    </div>

    <div class="table-wrap">
        <table>
            <thead><tr><th>ID</th><th>Device</th><th>Temp</th><th>Humidity</th><th>Pressure</th><th>Light</th><th>Timestamp</th></tr></thead>
            <tbody id="tbody"></tbody>
        </table>
    </div>

    <div class="footer">IoT Gateway v1.0 | Auto-refresh 3s</div>

    <script>
        function fetchData() {
            fetch('/api/data')
                .then(r => r.json())
                .then(d => {
                    if (d.latest) {
                        document.getElementById('temp').textContent  = d.latest.temp.toFixed(1);
                        document.getElementById('humi').textContent  = d.latest.humidity.toFixed(1);
                        document.getElementById('pres').textContent  = d.latest.pressure.toFixed(1);
                        document.getElementById('light').textContent = d.latest.light;
                    }
                    document.getElementById('count').textContent = d.count;

                    let html = '';
                    d.rows.forEach(r => {
                        html += `<tr>
                            <td>${r.id}</td><td>${r.device}</td>
                            <td>${r.temp.toFixed(1)}</td><td>${r.humidity.toFixed(1)}</td>
                            <td>${r.pressure.toFixed(1)}</td><td>${r.light}</td>
                            <td>${r.ts}</td></tr>`;
                    });
                    document.getElementById('tbody').innerHTML = html;
                })
                .catch(e => console.error(e));
        }
        fetchData();
        setInterval(fetchData, 3000);
    </script>
</body>
</html>
"""

def query_db(sql, args=()):
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    cur = conn.execute(sql, args)
    rows = cur.fetchall()
    conn.close()
    return rows

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/data')
def api_data():
    rows = query_db('SELECT * FROM sensor ORDER BY id DESC LIMIT 50')
    latest = query_db('SELECT * FROM sensor ORDER BY id DESC LIMIT 1')

    result = {
        'count': len(query_db('SELECT id FROM sensor')),
        'latest': None,
        'rows': []
    }

    if latest:
        r = latest[0]
        result['latest'] = {
            'temp': r['temp'], 'humidity': r['humidity'],
            'pressure': r['pressure'], 'light': r['light']
        }

    for r in rows:
        result['rows'].append({
            'id': r['id'], 'device': r['device'],
            'temp': r['temp'], 'humidity': r['humidity'],
            'pressure': r['pressure'], 'light': r['light'],
            'ts': r['ts']
        })

    return jsonify(result)

if __name__ == '__main__':
    print('[WEB] starting on http://0.0.0.0:5000')
    app.run(host='0.0.0.0', port=5000, debug=False)
