import streamlit as st
import requests
import random

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
flask_url_log = "http://8.141.82.206:5000/view_log"
flask_url_data = "http://8.141.82.206:5000/view_data"
flask_url_usr = "http://8.141.82.206:5000/view_usr"
flask_usl_manage = "http://8.141.82.206:5000/manage_usr"

# 从 Flask 应用获取数据
if add_selectbox == '查看日志':
    response_log = requests.get(flask_url_log)
    st.button('刷新')
    if response_log.status_code == 200:
        data = response_log.json()
        st.header('接收到的日志数据')
        # st.json(data)
        st.table(data)
    else:
        st.write('日志数据异常')

if add_selectbox == '查看当前读卡器状态':
    response_data = requests.get(flask_url_data)
    st.button('刷新读卡器')
    if response_data.status_code == 200:
        data = response_data.json()
        st.write('接收到的数据：读卡器实时数据')
        st.write(data[0]['decid'])
    else:
        st.write(response_data.text)

response_usr = requests.get(flask_url_usr)
usr_data = response_usr.json()
id_list = [row['DECID'] for row in usr_data]
if "decid_temp" not in st.session_state:
    st.session_state.decid_temp = 0
if add_selectbox == '用户管理':
    @st.experimental_fragment
    def usr_table():
        response_usr = requests.get(flask_url_usr)
        if response_usr.status_code == 200:
            usr_data = response_usr.json()
            id_list = [row['DECID'] for row in usr_data]
            st.header('用户数据表')
            st.table(usr_data)
        else:
            st.write('用户数据异常')
        if st.button('刷新数据表',key=random.random()):
            st.success('用户数据表已刷新！')
    usr_table()
    headers = {"Content-Type": "application/json"}
    if "password" not in st.session_state:
        st.session_state.password = ''
    st.session_state.password = st.text_input('管理员密码', type='password')
    disable = True
    if st.session_state.password == 'pkulock':
        st.success('管理员密码正确！')
        disable = False
    # with st.popover('用户管理',use_container_width=True,help='请输入管理员密码进行用户管理操作。',disabled=disable):
    if not disable:
        with st.expander('用户管理'):
            col1, col2, col3 = st.columns(3)
            with col1:
                @st.experimental_fragment
                def insert():
                    response_usr = requests.get(flask_url_usr)
                    usr_data = response_usr.json()
                    id_list = [row['DECID'] for row in usr_data]
                    st.header('插入数据')
                    textbox_name = '卡号'
                    if st.button('从读卡器读取卡号',key='insert'):
                        response_data = requests.get(flask_url_data)
                        if response_data.status_code == 200:
                            st.session_state.decid_temp = response_data.json()[0]['decid']   
                            textbox_name = '当前检测到卡号为'
                        else:
                            st.error('读卡器未检测到卡片！')
                            textbox_name = '上次检测到卡号为'
                    id_to_insert = st.text_input(textbox_name, value = st.session_state.decid_temp, placeholder = 0, key='insert-textbox')
                    status = st.selectbox('设置状态', ['永久用户', '临时用户'])
                    cnt = 0 
                    if status == '永久用户':
                        cnt = -1
                    else:
                        cnt = st.number_input('设置可用次数', min_value=1, step=1)
                    if st.button('插入'):
                        if id_to_insert in id_list:
                            st.error('卡号已存在！')        
                        elif id_to_insert == '':
                            st.error('卡号为空！')
                        else:
                            data = {
                                "method": "insert",
                                "decid": id_to_insert,
                                "status": status,
                                "count": cnt
                            }
                            with st.spinner('Wait for it...'):
                                response = requests.post(flask_usl_manage, json=data, headers=headers)
                                if response.status_code == 200:
                                    st.success(response.text)
                                else:
                                    st.error('服务器错误！')
                insert()
            with col2:
                @st.experimental_fragment
                def update():
                    response_usr = requests.get(flask_url_usr)
                    usr_data = response_usr.json()
                    id_list = [row['DECID'] for row in usr_data]
                    st.header('更新数据')
                    textbox_name = '卡号'
                    if st.button('从读卡器读取卡号',key='update'):
                        response_data = requests.get(flask_url_data)
                        if response_data.status_code == 200:
                            st.session_state.decid_temp = response_data.json()[0]['decid']   
                            textbox_name = '当前检测到卡号为'
                        else:
                            st.error('读卡器未检测到卡片！')
                            textbox_name = '上次检测到卡号为'
                    id_to_update = st.text_input(textbox_name, value = st.session_state.decid_temp, placeholder = 0, key='update-textbox')
                    status = st.selectbox('修改状态', ['永久用户', '临时用户'])
                    cnt = 0
                    if status == '永久用户':
                        cnt = -1
                    else:
                        cnt = st.number_input('更新可用次数', min_value=1, step=1)
                    if st.button('更新'):
                        if id_to_update not in id_list:
                            st.error('卡号不存在！')
                        else:
                            data = {
                                "method": "update",
                                "decid": id_to_update,
                                "status": status,
                                "count": cnt
                            }            
                            with st.spinner('Wait for it...'):
                                response = requests.post(flask_usl_manage, json=data, headers=headers)
                                if response.status_code == 200:
                                    st.success(response.text)
                                else:
                                    st.error('服务器错误！')
                update()

            with col3:
                @st.experimental_fragment
                def delete():
                    response_usr = requests.get(flask_url_usr)
                    usr_data = response_usr.json()
                    id_list = [row['DECID'] for row in usr_data]
                    st.header('删除数据')
                    st.button('刷新用户表')
                    id_to_delete = st.selectbox('目标卡号', id_list)
                    if st.button('删除'):
                        data = {
                            "method": "delete",
                            "decid": id_to_delete,
                            "status": " ",
                            "count": 0
                        }
                        with st.spinner('Wait for it...'):
                            response = requests.post(flask_usl_manage, json=data, headers=headers)
                            if response.status_code == 200:
                                st.success(response.text)
                            else:
                                st.error('服务器错误！')
                delete()
                @st.experimental_fragment
                def delete_all():
                    if st.button('删除所有数据'):
                        data = {
                            "method": "delete_all",
                            "decid": 0,
                            "status": " ",
                            "count": 0
                        }
                        with st.spinner('Wait for it...'):
                            response = requests.post(flask_usl_manage, json=data, headers=headers)
                            if response.status_code == 200:
                                st.success(response.text)
                            else:
                                st.error('服务器错误！')
                delete_all()

st.sidebar.markdown('---')
st.sidebar.markdown('**智能电子系统设计与实践2024**')
st.sidebar.markdown('- 智能门锁云端服务平台')
st.sidebar.markdown('---')
st.sidebar.markdown('**关注我们**')
st.sidebar.markdown('[GitHub](https://github.com/scc2450/smartlock_app)')
st.sidebar.markdown('---')
st.sidebar.markdown('**项目元件**')
st.sidebar.markdown('[Streamlit](https://streamlit.io)')
st.sidebar.markdown('[Flask](https://flask.palletsprojects.com)')
st.sidebar.markdown('[SQLite](https://www.sqlite.org)')
st.sidebar.markdown('Esp32 Wemos Lolin32 Lite')
st.sidebar.markdown('MFRC522 RFID Reader')
st.sidebar.markdown('FPM383D Fingerprint Sensor')
st.sidebar.markdown('---')
st.sidebar.markdown('**联系我们**')
st.sidebar.markdown('北京市海淀区颐和园路5号')
st.sidebar.markdown('北京大学信息科学技术学院1231实验室')
st.sidebar.markdown('---')