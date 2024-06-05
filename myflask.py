from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__) # 创建Flask应用
CORS(app)  # 启用跨域资源共享，以便从其他域名访问服务器，比如Streamlit应用可以从本地服务器获取数据

# 数据存储变量
data = {}

# 创建数据库sqlite3
import sqlite3, json
conn = sqlite3.connect('log.db')
c = conn.cursor()
c.execute('''
        CREATE TABLE IF NOT EXISTS data (
            id INTEGER PRIMARY KEY,
            decid INTEGER,
            status TEXT,
            method TEXT
        )
    ''') # 加入新行时记得在行尾加上逗号
conn.commit()
conn.close()

# 接收ESP32数据包的路由
@app.route('/receive_data', methods=['POST'])
def receive_data():
    global data
    # 从请求中获取JSON数据
    json_data = request.get_json()
    # 存储数据
    data = json_data
    # 判断值
    if 'decid' in data:
        data['status'] = 'invalid'
        # 存储数据到数据库
        conn = sqlite3.connect('usr.db')
        c = conn.cursor()
        if c.execute('SELECT * FROM data WHERE decid = ? AND status = ?', (data['decid'],'永久用户')).fetchone(): # 查询数据库中是否有相同的decid,fetchone()返回一条数据
            data['status'] = 'valid'
        if c.execute('SELECT * FROM data WHERE decid = ? AND status = ?', (data['decid'],'临时用户')).fetchone():
            cnt = c.execute('SELECT count FROM data WHERE decid = ? AND status = ?', (data['decid'],'临时用户')).fetchone()[0]
            data['status'] = 'temp-valid' if cnt > 0 else 'temp-invalid'
            c.execute('UPDATE data SET count = ? WHERE decid = ?', (max(cnt-1,0), data['decid']))
            conn.commit()
        conn.close()
        conn = sqlite3.connect('log.db')
        c = conn.cursor()
        c.execute('''
            INSERT INTO data (decid, status, method)
            VALUES (?, ?, ?)
        ''', (data['decid'],data['status'],'card'))
        conn.commit()
        conn.close()
        if data['status'] == 'valid' or data['status'] == 'temp-valid':
            return jsonify({"message": "Data received, vadilation passed"}), 200 # 返回状态码200表示成功
        return jsonify({"message": "Data received, vadilation failed"}), 201 # 返回状态码201表示认证失败
    elif 'fingerID' in data:        
        data['status'] = 'valid' if data['fingerID'] > 0 else 'invalid' 
        conn = sqlite3.connect('log.db')
        c = conn.cursor()
        c.execute('''
            INSERT INTO data (decid, status, method)
            VALUES (?, ?, ?)
        ''', (data['fingerID'],data['status'],'finger'))
        conn.commit()
        conn.close()
        return jsonify({"message": "Data received"}), 200
    elif 'faceID' in data:        
        data['status'] = 'valid' if data['faceID'] > 0 else 'invalid' 
        conn = sqlite3.connect('log.db')
        c = conn.cursor()
        c.execute('''
            INSERT INTO data (decid, status, method)
            VALUES (?, ?, ?)
        ''', (data['faceID'],data['status'],'face'))
        conn.commit()
        conn.close()
        return jsonify({"message": "Data received"}), 200
    else:
        return jsonify({"message": "Data received, but method missed."}), 202

# 查看日志数据的路由
@app.route('/view_log', methods=['GET'])
def view_log():
    log = []

    conn = sqlite3.connect('log.db')
    cursor = conn.cursor()

    # 查询所有数据
    cursor.execute('SELECT * FROM data')
    rows = cursor.fetchall()
    # rows = rows[-6:] # 只显示最新的6条数据
    rows = rows[::-1] # 倒序显示

    # 将数据转换为 JSON 格式
    log = [{'NO.': row[0], 'DECID': row[1], 'STATUS':row[2], 'METHOD':row[3]} for row in rows]

    conn.close()
    # 返回数据的JSON格式
    return jsonify(log), 200

# 查看当前数据的路由
@app.route('/view_data', methods=['GET'])
def view_data():
    global data
    if 'decid' in data:
        data_decid = [{'decid':data['decid']}]
    else:
        return jsonify({"message": "No data received yet."}), 203

    conn = sqlite3.connect('log.db')
    cursor = conn.cursor()

    # 查询所有数据
    cursor.execute('SELECT * FROM data')
    rows = cursor.fetchall()

    # 将数据转换为 JSON 格式
    data = [{'ID': row[0], 'DECID': row[1], 'STATUS':row[2]} for row in rows]

    conn.close()
    # 返回数据的JSON格式
    return jsonify(data_decid), 200

if __name__ == "__main__":
    app.run(port=5000, host='0.0.0.0')  # 确保服务器在局域网内可访问
