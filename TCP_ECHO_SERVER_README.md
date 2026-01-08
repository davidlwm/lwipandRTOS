# TCP Echo Server 测试说明

## 项目配置

### 网络配置
- **静态IP**: 192.168.1.30
- **子网掩码**: 255.255.255.0
- **网关**: 192.168.1.1
- **Echo Server 端口**: 7

### 串口配置（调试日志）
- **串口**: USART1
- **引脚**: TX=PA9, RX=PA10
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无

## 编译和下载

1. 打开 Keil 项目：`MDK-ARM/Desktop.uvprojx`
2. 编译项目：按 F7
3. 下载到板子：按 F8

## 测试方法

### 1. 使用 telnet 测试

```bash
telnet 192.168.1.30 7
```

输入任意文本，服务器会回显相同的内容。

### 2. 使用 netcat (nc) 测试

```bash
nc 192.168.1.30 7
```

或者发送文件内容：

```bash
echo "Hello, Echo Server!" | nc 192.168.1.30 7
```

### 3. 使用 Python 测试

```python
import socket

# 创建 TCP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 连接到 echo server
sock.connect(('192.168.1.30', 7))
print("已连接到 Echo Server")

# 发送数据
message = "Hello from Python!"
sock.sendall(message.encode())
print(f"发送: {message}")

# 接收回显数据
data = sock.recv(1024)
print(f"接收: {data.decode()}")

# 关闭连接
sock.close()
print("连接已关闭")
```

## 日志输出

通过串口（USART1, 115200bps）可以看到以下日志：

### 启动日志
```
=== STM32F407 lwIP + RTOS TCP Echo Server ===
系统启动中...
[TCP Echo Server] 初始化成功，监听端口 7
[TCP Echo Server] 静态IP: 192.168.1.30
```

### 连接日志
```
[TCP Echo Server] 新连接建立 - 客户端IP: 192.168.1.100:54321
```

### 数据接收日志
```
[TCP Echo Server] 收到数据 - 长度: 18 字节，来自: 192.168.1.100:54321
[TCP Echo Server] 数据内容: Hello, Echo Server!
[TCP Echo Server] 数据已回显
```

### 断开连接日志
```
[TCP Echo Server] 客户端关闭连接 - IP: 192.168.1.100:54321
[TCP Echo Server] 连接已关闭
```

## 功能特性

1. **完整的连接管理**
   - 新连接建立时记录客户端 IP 和端口
   - 连接断开时记录日志

2. **数据回显**
   - 接收客户端发送的数据
   - 原样回显给客户端
   - 记录数据长度和内容（前64字节）

3. **错误处理**
   - 内存分配失败处理
   - 网络错误处理
   - 连接异常处理

4. **详细日志**
   - 所有网络事件都有中文日志输出
   - 便于调试和监控

## 故障排查

### 无法连接
1. 检查网线是否连接
2. 检查 IP 地址是否在同一网段
3. 使用 `ping 192.168.1.30` 测试网络连通性
4. 检查防火墙设置

### 无日志输出
1. 检查串口连接（PA9, PA10）
2. 确认串口工具波特率设置为 115200
3. 检查 USART1 初始化是否成功

### 数据不回显
1. 查看串口日志确认是否收到数据
2. 检查 lwIP 配置
3. 确认 TCP 缓冲区设置

## 文件清单

### 新增文件
- `Core/Inc/tcp_echo_server.h` - Echo server 头文件
- `Core/Src/tcp_echo_server.c` - Echo server 实现
- `Core/Inc/retarget.h` - Printf 重定向头文件
- `Core/Src/retarget.c` - Printf 重定向实现

### 修改文件
- `Core/Src/main.c` - 添加 USART1 和 Echo server 初始化
- `MDK-ARM/Desktop.uvprojx` - 添加新文件到 Keil 项目

## 技术细节

- **协议**: TCP
- **端口**: 7 (标准 echo 端口)
- **RTOS**: FreeRTOS
- **TCP/IP 栈**: lwIP 2.1.2
- **回显模式**: 立即回显（收到数据后立即发送回客户端）
