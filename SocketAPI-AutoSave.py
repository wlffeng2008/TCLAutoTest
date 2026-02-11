#Please use python3
import time
import socket

input('press inter to connect.')
client = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
client.connect(('127.0.0.1', 23367)) #默认本机IP+端口23367，可根据VIS软件配置自行修改

n = 0
while True:
    cmd = 'start' #如未连接硬件设备则使用'start --simulate'
    client.send(cmd.encode('utf-8'))
    result = client.recv(2048).decode('utf-8')
    if result.find('NAK') == 0:
        print('启动失败！')
    else:
        n = n + 1
        cmd = 'export-data E:\\temp\\test' + str(n) + '.kvdat' #默认保存到"E:\temp\"目录下，可自行修改路径及文件名
        client.send(cmd.encode('utf-8'))
        result = client.recv(2048).decode('utf-8')
        if result.find('NAK') == 0:
            print('保存失败！')
        else:
            print('成功执行' + str(n) + '次')
    time.sleep(0.5)

client.close()
