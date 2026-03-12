#!/usr/bin/env python3
import socket
import struct
import time

def create_login_request(username, password):
    """创建登录请求"""
    # 登录请求 protobuf 格式
    # 消息类型: LOGIN = 1
    msg_type = 1
    
    # 简单的消息体: username|password
    body = f"{username}|{password}".encode('utf-8')
    
    # 打包: 消息类型(4字节) + 消息长度(4字节) + 消息体
    header = struct.pack('!II', msg_type, len(body))
    return header + body

def test_server():
    """测试服务器连接"""
    host = '127.0.0.1'
    port = 8888
    
    print(f"Connecting to {host}:{port}...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    
    try:
        sock.connect((host, port))
        print("Connected successfully!")
        
        # 发送登录请求
        username = "testuser"
        password = "testpass"
        print(f"Sending login request for user: {username}")
        
        request = create_login_request(username, password)
        sock.sendall(request)
        print(f"Sent {len(request)} bytes")
        
        # 接收响应
        response = sock.recv(1024)
        print(f"Received {len(response)} bytes")
        print(f"Response (hex): {response.hex()}")
        
        # 解析响应头
        if len(response) >= 8:
            msg_type = struct.unpack('!I', response[0:4])[0]
            msg_len = struct.unpack('!I', response[4:8])[0]
            print(f"Response type: {msg_type}, length: {msg_len}")
            
            if len(response) > 8:
                body = response[8:8+msg_len]
                print(f"Response body: {body.decode('utf-8', errors='ignore')}")
        
        print("\nTest completed successfully!")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()
        print("Connection closed")

if __name__ == "__main__":
    test_server()