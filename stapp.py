import streamlit as st
import requests

st.title('智能门锁云端服务平台')
add_selectbox = st.sidebar.selectbox(
    "您希望进行的操作？",
    ("查看日志", "查看当前读卡器状态", "用户管理")
)

PKU = "images/pku.png"
NAME_CN = "images/中文校名_红色.png"
NAME_CEN = "images/中英文校名_红色.png"
LOGO_NAME = "images/标志与中英文校名组合规范_左右.png"

options = [PKU, NAME_CN, NAME_CEN, LOGO_NAME]
sidebar_logo = NAME_CN
main_body_logo = LOGO_NAME
st.logo(sidebar_logo, icon_image=main_body_logo)

# 定义 Flask 应用的 URL
flask_url_log = "http://192.168.43.131:5000/view_log"
flask_url_data = "http://192.168.43.131:5000/view_data"

# 从 Flask 应用获取数据
if add_selectbox == '查看日志':
    st.button('刷新')
    response_log = requests.get(flask_url_log)
    if response_log.status_code == 200:
        data = response_log.json()
        st.header('接收到的日志数据')
        # st.json(data)
        st.table(data)
    else:
        st.write('当前日志为空。')

response_data = requests.get(flask_url_data)
if add_selectbox == '查看当前读卡器状态':
    st.button('刷新读卡器')
    if response_data.status_code == 200:
        data = response_data.json()
        st.write('接收到的数据：读卡器实时数据')
        st.write(data[0]['decid'])
    else:
        st.write(response_data.text)

if add_selectbox == '用户管理':
    import sqlite3
    conn = sqlite3.connect('usr.db')
    cursor = conn.cursor()
    cursor.execute('''
            CREATE TABLE IF NOT EXISTS data (
                id INTEGER PRIMARY KEY,
                decid TEXT,
                status TEXT,
                count INTEGER
            )
        ''')
    
    # 显示数据表
    st.header('用户数据表')
    cursor.execute('SELECT * FROM data')
    rows = cursor.fetchall()
    data = [{'ID': row[0], 'DECID': row[1], 'STATUS':row[2], 'CNT':row[3]} for row in rows]
    # st.write(data)
    st.table(data)

    # 刷新数据表
    if st.button('刷新数据表'):
        st.success('用户数据表已刷新！')

    # 输入管理员密码
    password = st.text_input('请输入管理员密码来进行用户数据管理', type='password')
    confirm_password = st.button('确认')
    if confirm_password and password == 'smartlock' :
        st.success('密码正确！')
        col1, col2, col3 = st.columns(3)
        with col1:
            st.header('插入数据')
            decid_temp = 0
            st.button('从读卡器读取卡号',key='insert')
            if response_data.status_code == 200:
                decid_temp = response_data.json()[0]['decid']   
            decid = st.text_input('卡号', value = decid_temp, placeholder = decid_temp, key='insert-textbox')
            status = st.selectbox('设置状态', ['永久用户', '临时用户'])
            cnt = 0 
            if status == '永久用户':
                cnt = -1
            else:
                cnt = st.number_input('设置可用次数', min_value=1, step=1)
            if st.button('插入'):
                list = [row[1] for row in rows]
                if decid in list:
                    conn.commit()
                    conn.close()
                    st.error('卡号已存在！')        
                elif decid == '':
                    conn.commit()
                    conn.close()
                    st.error('卡号为空！')
                else:
                    cursor.execute('''
                        INSERT INTO data (decid, status, count)
                        VALUES (?, ?, ?)
                    ''', (decid,status,cnt))
                    conn.commit()
                    conn.close()
                    st.success('数据插入成功！')

        with col2:
            st.header('更新可用次数')
            st.button('从读卡器读取卡号', key='update')
            if response_data.status_code == 200:
                decid_temp = response_data.json()[0]['decid']   
            id_to_update = st.text_input('卡号', value = decid_temp, placeholder = decid_temp, key='update-textbox')
            status = st.selectbox('修改状态', ['永久用户', '临时用户'])
            cnt = 0
            if status == '永久用户':
                cnt = -1
            else:
                cnt = st.number_input('更新可用次数', min_value=1, step=1)
            if st.button('更新'):
                cursor.execute('''
                    UPDATE data SET count = ?, status = ? WHERE decid = ?
                ''', (cnt, status, id_to_update))
                conn.commit()
                conn.close()
                st.success('数据更新成功！')

        with col3:
            st.header('删除数据')
            id_to_delete = st.selectbox('目标卡号', [row[1] for row in rows])
            if st.button('删除'):
                cursor.execute('''
                    DELETE FROM data WHERE decid = ?
                ''', (id_to_delete,))
                conn.commit()
                conn.close()
                st.success('数据删除成功！')

            if st.button('删除所有数据'):
                cursor.execute('''
                    DELETE FROM data
                ''')
                conn.commit()
                conn.close()
                st.warning('所有数据已删除！')
        # data = pandas.DataFrame(data)
        # options_builder = GridOptionsBuilder.from_dataframe(data)
        # options_builder.configure_default_column(groupable=True, value=True, enableRowGroup=True, aggFunc='sum', editable=True, wrapText=True, autoHeight=True)
        # grid_options = options_builder.build()
        # grid_return = AgGrid(data, grid_options)
    else:
        if confirm_password and password != '':
            st.error('密码错误！')

