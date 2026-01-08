# TCP Echo Server 完整日志示例

## 预期的完整启动日志

```
========================================
  STM32F407 lwIP + RTOS TCP Echo Server
========================================
System starting...
USART1 initialized: TX=PA9, RX=PA10, 115200bps

[Task] Default task started
[Task] Initializing lwIP stack...

[lwIP] Network Configuration:
[lwIP]   IP Address: 192.168.1.30
[lwIP]   Netmask:    255.255.255.0
[lwIP]   Gateway:    192.168.1.1
[lwIP] Network interface is UP
[lwIP] Ethernet link thread created
[Task] lwIP initialized successfully
[Task] Waiting for network link...

[lwIP] *** Ethernet Link UP ***
[lwIP] Network is ready to accept connections

[Task] Starting TCP Echo Server...

[TCP Echo Server] ========================================
[TCP Echo Server] Initialization successful!
[TCP Echo Server] Listening on port 7
[TCP Echo Server] Static IP: 192.168.1.30
[TCP Echo Server] Waiting for client connections...
[TCP Echo Server] ========================================

[Task] System ready, entering main loop
========================================
```

## 客户端连接时的日志

当客户端连接时（例如 `telnet 192.168.1.30 7`）：

```
>>> [TCP Echo Server] NEW CONNECTION <<<
[TCP Echo Server] Client IP: 192.168.1.100:54321
[TCP Echo Server] Connection established
```

## 接收和回显数据的日志

当客户端发送 "Hello World" 时：

```
[TCP Echo Server] <<< DATA RECEIVED <<<
[TCP Echo Server] From: 192.168.1.100:54321
[TCP Echo Server] Length: 13 bytes
[TCP Echo Server] Content: "Hello World\r\n"
[TCP Echo Server] >>> DATA ECHOED >>> (13 bytes sent back)
```

## 客户端断开连接的日志

当客户端关闭连接时：

```
<<< [TCP Echo Server] CONNECTION CLOSED <<<
[TCP Echo Server] Client IP: 192.168.1.100:54321
[TCP Echo Server] Connection terminated by client
```

## 完整的测试会话示例

```
========================================
  STM32F407 lwIP + RTOS TCP Echo Server
========================================
System starting...
USART1 initialized: TX=PA9, RX=PA10, 115200bps

[Task] Default task started
[Task] Initializing lwIP stack...

[lwIP] Network Configuration:
[lwIP]   IP Address: 192.168.1.30
[lwIP]   Netmask:    255.255.255.0
[lwIP]   Gateway:    192.168.1.1
[lwIP] Network interface is UP
[lwIP] Ethernet link thread created
[Task] lwIP initialized successfully
[Task] Waiting for network link...

[lwIP] *** Ethernet Link UP ***
[lwIP] Network is ready to accept connections

[Task] Starting TCP Echo Server...

[TCP Echo Server] ========================================
[TCP Echo Server] Initialization successful!
[TCP Echo Server] Listening on port 7
[TCP Echo Server] Static IP: 192.168.1.30
[TCP Echo Server] Waiting for client connections...
[TCP Echo Server] ========================================

[Task] System ready, entering main loop
========================================


>>> [TCP Echo Server] NEW CONNECTION <<<
[TCP Echo Server] Client IP: 192.168.1.100:54321
[TCP Echo Server] Connection established

[TCP Echo Server] <<< DATA RECEIVED <<<
[TCP Echo Server] From: 192.168.1.100:54321
[TCP Echo Server] Length: 6 bytes
[TCP Echo Server] Content: "test\r\n"
[TCP Echo Server] >>> DATA ECHOED >>> (6 bytes sent back)

[TCP Echo Server] <<< DATA RECEIVED <<<
[TCP Echo Server] From: 192.168.1.100:54321
[TCP Echo Server] Length: 13 bytes
[TCP Echo Server] Content: "Hello World\r\n"
[TCP Echo Server] >>> DATA ECHOED >>> (13 bytes sent back)

<<< [TCP Echo Server] CONNECTION CLOSED <<<
[TCP Echo Server] Client IP: 192.168.1.100:54321
[TCP Echo Server] Connection terminated by client


>>> [TCP Echo Server] NEW CONNECTION <<<
[TCP Echo Server] Client IP: 192.168.1.101:12345
[TCP Echo Server] Connection established

[TCP Echo Server] <<< DATA RECEIVED <<<
[TCP Echo Server] From: 192.168.1.101:12345
[TCP Echo Server] Length: 20 bytes
[TCP Echo Server] Content: "Another test message"
[TCP Echo Server] >>> DATA ECHOED >>> (20 bytes sent back)

<<< [TCP Echo Server] CONNECTION CLOSED <<<
[TCP Echo Server] Client IP: 192.168.1.101:12345
[TCP Echo Server] Connection terminated by client
```

## 测试命令

### 使用 telnet
```bash
telnet 192.168.1.30 7
# 输入任意文本，按回车
# 应该看到回显
```

### 使用 netcat
```bash
echo "test message" | nc 192.168.1.30 7
```

### 使用 Python
```python
import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.30', 7))
print("Connected!")

# 发送数据
sock.sendall(b"Hello Echo Server\n")

# 接收回显
response = sock.recv(1024)
print(f"Received: {response.decode()}")

sock.close()
```

## 日志说明

### 日志前缀含义
- `[Task]` - FreeRTOS 任务相关日志
- `[lwIP]` - lwIP 网络协议栈日志
- `[TCP Echo Server]` - Echo Server 应用日志

### 特殊标记
- `>>>` - 输出/发送操作
- `<<<` - 输入/接收操作
- `***` - 重要事件

### 数据内容显示
- 可打印字符：直接显示
- `\r` - 回车符
- `\n` - 换行符
- `.` - 不可打印字符
- 超过 64 字节的数据会显示 `...`

## 故障排查

### 没有启动日志
- 检查串口连接和波特率（115200）
- 确认 Keil 中启用了 MicroLIB
- 检查 USART1 初始化

### 没有网络日志
- 检查网线连接
- 等待几秒让 PHY 芯片初始化
- 查看是否有 "Ethernet Link UP" 消息

### 无法连接 Echo Server
- 使用 `ping 192.168.1.30` 测试连通性
- 确认 PC 和板子在同一网段
- 检查防火墙设置
- 确认看到 "Waiting for client connections" 消息

### 连接成功但没有回显
- 查看串口是否有 "DATA RECEIVED" 日志
- 查看是否有 "DATA ECHOED" 日志
- 检查是否有错误日志
