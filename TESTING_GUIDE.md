# TCP Echo Server 测试指南

## 预期的串口日志输出

连接串口（PA9/PA10, 115200bps）后，应该看到以下日志：

```
========================================
  STM32F407 lwIP + RTOS TCP Echo Server
========================================
System starting...
USART1 initialized: TX=PA9, RX=PA10, 115200bps

[Task] Default task started
[Task] Initializing lwIP stack...
[lwIP] Network configuration:
[lwIP]   IP Address: 192.168.1.30
[lwIP]   Netmask:    255.255.255.0
[lwIP]   Gateway:    192.168.1.1
[lwIP] Network interface configured and up
[lwIP] Ethernet link thread created
[Task] lwIP initialized
[Task] Waiting for network to be ready...
[lwIP] Network link is UP
[lwIP] Ready to accept connections
[Task] Network ready, starting TCP Echo Server...
[TCP Echo Server] Init OK, listening on port 7
[TCP Echo Server] Static IP: 192.168.1.30
[Task] Echo server initialized, entering main loop
```

## 测试 Echo Server

### 方法1：使用 telnet

```bash
telnet 192.168.1.30 7
```

输入任意文本，按回车，应该看到：
- 串口日志显示连接和数据接收
- telnet 窗口显示回显的数据

**预期串口日志：**
```
[TCP Echo Server] New connection - Client IP: 192.168.1.100:54321
[TCP Echo Server] Received data - Length: 10 bytes, from: 192.168.1.100:54321
[TCP Echo Server] Data content: test data
[TCP Echo Server] Data echoed
```

### 方法2：使用 netcat (nc)

```bash
echo "Hello Echo Server" | nc 192.168.1.30 7
```

### 方法3：使用 Python 脚本

```python
import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.30', 7))
print("Connected to Echo Server")

# 发送测试数据
test_data = "Hello from Python!"
sock.sendall(test_data.encode())
print(f"Sent: {test_data}")

# 接收回显
response = sock.recv(1024)
print(f"Received: {response.decode()}")

sock.close()
print("Connection closed")
```

## 故障排查

### 1. 没有任何串口输出

**可能原因：**
- 串口线没连接或连接错误
- 串口工具波特率不是 115200
- PA9/PA10 引脚配置错误

**解决方法：**
- 检查串口连接：TX=PA9, RX=PA10
- 确认波特率：115200, 8N1
- 检查串口工具是否打开正确的 COM 口

### 2. 有启动日志但没有网络日志

**可能原因：**
- 网线没连接
- PHY 芯片没有初始化
- 以太网时钟配置错误

**解决方法：**
- 检查网线连接
- 检查 PHY 芯片供电
- 查看是否有 `[lwIP] Network link is UP` 日志

### 3. 网络正常但无法连接 Echo Server

**可能原因：**
- IP 地址不在同一网段
- 防火墙阻止
- Echo Server 初始化失败

**解决方法：**
- 确认 PC 和板子在同一网段（192.168.1.x）
- 使用 `ping 192.168.1.30` 测试连通性
- 检查是否有 `[TCP Echo Server] Init OK` 日志

### 4. 能连接但没有回显

**可能原因：**
- TCP 接收回调没有触发
- 数据发送失败

**解决方法：**
- 查看串口日志是否有 `Received data` 消息
- 查看是否有 `Data echoed` 消息
- 检查是否有错误日志

## 完整的测试流程

1. **编译下载**
   - 打开 Keil 项目
   - 按 F7 编译
   - 按 F8 下载

2. **连接串口**
   - 连接串口工具到 PA9/PA10
   - 设置波特率 115200
   - 复位板子，查看启动日志

3. **连接网线**
   - 将板子连接到路由器或交换机
   - 确保 PC 也在同一网络

4. **测试连通性**
   ```bash
   ping 192.168.1.30
   ```

5. **测试 Echo Server**
   ```bash
   telnet 192.168.1.30 7
   # 或
   echo "test" | nc 192.168.1.30 7
   ```

6. **观察日志**
   - 串口应该显示连接、接收、回显的日志
   - 客户端应该收到回显的数据

## 日志级别说明

- `[Task]` - FreeRTOS 任务相关
- `[lwIP]` - 网络协议栈相关
- `[TCP Echo Server]` - Echo Server 应用相关

所有日志都会通过 USART1 (PA9/PA10) 输出。
