#Please use python3
import time
import socket

client = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
while True:
    ip = input('请输入目标IP地址 (直接回车则设为本机地址：127.0.0.1): ')
    if len(ip)==0:
        ip = '127.0.0.1'
    print('目标IP：' + ip)
    port = input('请输入目标端口 (直接回车则设为默认端口：23367): ')
    if len(port)==0:
        port = '23367'
    print('目标端口：' + port)
    try:
        client.connect((ip, int(port)))
        print('连接成功！\n')
        break
    except Exception as err:
        print('连接失败！(请确保VIS软件已开启Socket功能、IP及端口设置正确)\n')

while True:
    cmd = input('请输入测试命令: ')
    if len(cmd)>0:
        print(cmd)
        client.send(cmd.encode('utf-8'))
        result = client.recv(2048).decode('utf-8')
        if result.find('NAK')==0:
            print('执行失败！\n')
        else:
            print('执行成功：' + result + '\n')
    time.sleep(0.5)

client.close()
