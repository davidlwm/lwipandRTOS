# TCP Echo Server 故障排查指南

## 问题：日志在 "Ethernet link thread created" 后停止

### 症状
```
[lwIP] Network interface is UP
[lwIP] Ethernet link thread created
```
之后没有任何日志输出。

### 可能原因

#### 1. 任务栈溢出
**检查方法：**
- 在 Keil 中查看 `defaultTask` 的栈大小
- 默认可能只有 128*4 = 512 字节

**解决方法：**
在 `main.c` 中增加任务栈大小：
```c
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,  // 改为 512*4 = 2048 字节
  .priority = (osPriority_t) osPriorityNormal,
};
```

#### 2. lwIP 内存不足
**检查方法：**
查看 `lwipopts.h` 中的内存配置

**解决方法：**
增加 lwIP 内存池大小（在 `LWIP/Target/lwipopts.h`）：
```c
#define MEM_SIZE                (10*1024)  // 增加到 10KB
#define MEMP_NUM_TCP_PCB        10         // 增加 TCP PCB 数量
#define MEMP_NUM_TCP_PCB_LISTEN 8          // 增加监听 PCB 数量
```

#### 3. printf 导致死锁
**检查方法：**
- 如果在中断中调用 printf 可能导致死锁
- lwIP 回调函数中使用 printf 可能有问题

**解决方法：**
暂时注释掉 `lwip.c` 中的 printf，只保留 main.c 中的：
```c
// 在 lwip.c 中暂时注释掉所有 printf
```

#### 4. osDelay 时间过长导致看起来卡住
**检查方法：**
等待 2 秒以上，看是否有后续日志

**解决方法：**
已经在代码中添加了 "Delay complete" 日志来确认

## 当前添加的调试信息

### 1. 任务执行流程
```
[Task] Waiting for network link (2 seconds)...
[Task] Delay complete, checking network status...
[Task] Starting TCP Echo Server...
```

### 2. Echo Server 初始化详情
```
[TCP Echo Server] Starting initialization...
[TCP Echo Server] TCP PCB created: 0xXXXXXXXX
[TCP Echo Server] Binding to port 7...
[TCP Echo Server] Bind result: 0
[TCP Echo Server] Listen PCB: 0xXXXXXXXX
```

### 3. 心跳日志
每 10 秒输出一次心跳，确认任务在运行：
```
[Task] Heartbeat: 10 seconds
[Task] Heartbeat: 20 seconds
```

## 调试步骤

### 步骤 1：重新编译下载
1. 清理项目（Project → Clean）
2. 重新编译（F7）
3. 下载（F8）
4. 复位板子

### 步骤 2：观察完整日志
等待至少 5 秒，记录所有输出：
- 是否看到 "Delay complete"？
- 是否看到 "Starting initialization"？
- 是否看到 "TCP PCB created"？
- 是否看到 "Bind result"？

### 步骤 3：检查错误码
如果看到 "Bind result: X"，X 的含义：
- `0` (ERR_OK) - 成功
- `-1` (ERR_MEM) - 内存不足
- `-2` (ERR_BUF) - 缓冲区错误
- `-3` (ERR_TIMEOUT) - 超时
- `-4` (ERR_RTE) - 路由错误
- `-5` (ERR_INPROGRESS) - 操作进行中
- `-6` (ERR_VAL) - 非法值
- `-7` (ERR_WOULDBLOCK) - 操作会阻塞
- `-8` (ERR_USE) - 地址正在使用
- `-11` (ERR_ISCONN) - 已连接
- `-12` (ERR_ABRT) - 连接中止

### 步骤 4：增加任务栈
如果日志在某处突然停止，很可能是栈溢出。

在 `main.c` 中找到：
```c
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,  // <-- 这里
  .priority = (osPriority_t) osPriorityNormal,
};
```

改为：
```c
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,  // 增加到 4KB
  .priority = (osPriority_t) osPriorityNormal,
};
```

### 步骤 5：测试连接
如果看到完整的初始化日志，测试连接：

```bash
# 测试 ping
ping 192.168.1.30

# 测试端口（Linux/Mac）
nc -zv 192.168.1.30 7

# 测试端口（Windows）
Test-NetConnection -ComputerName 192.168.1.30 -Port 7

# 尝试连接
telnet 192.168.1.30 7
```

## 预期的完整日志

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
[Task] Waiting for network link (2 seconds)...

[lwIP] *** Ethernet Link UP ***
[lwIP] Network is ready to accept connections

[Task] Delay complete, checking network status...
[Task] Starting TCP Echo Server...
[TCP Echo Server] Starting initialization...
[TCP Echo Server] TCP PCB created: 0x20001234
[TCP Echo Server] Binding to port 7...
[TCP Echo Server] Bind result: 0
[TCP Echo Server] Listen PCB: 0x20001234

[TCP Echo Server] ========================================
[TCP Echo Server] Initialization successful!
[TCP Echo Server] Listening on port 7
[TCP Echo Server] Static IP: 192.168.1.30
[TCP Echo Server] Waiting for client connections...
[TCP Echo Server] ========================================

[Task] Echo Server initialization complete
[Task] System ready, entering main loop
========================================

[Task] Heartbeat: 10 seconds
[Task] Heartbeat: 20 seconds
```

## 如果还是不通

### 检查防火墙
```bash
# Windows
netsh advfirewall firewall add rule name="Echo Test" dir=in action=allow protocol=TCP localport=7

# Linux
sudo iptables -A INPUT -p tcp --dport 7 -j ACCEPT
```

### 使用 Wireshark 抓包
1. 在 PC 上启动 Wireshark
2. 过滤器：`tcp.port == 7`
3. 尝试连接 `telnet 192.168.1.30 7`
4. 查看是否有 SYN 包发出
5. 查看板子是否回复 SYN-ACK

### 检查网络配置
确保 PC 和板子在同一网段：
- 板子：192.168.1.30
- PC：192.168.1.x（例如 192.168.1.100）
- 子网掩码：255.255.255.0
