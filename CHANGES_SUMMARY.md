# TCP Echo Server 项目修改总结

## 新增文件列表

### 1. TCP Echo Server 实现
- **Core/Src/tcp_echo_server.c** - TCP Echo Server 实现（已添加到 uvprojx）
- **Core/Inc/tcp_echo_server.h** - TCP Echo Server 头文件

### 2. Printf 串口重定向
- **Core/Src/retarget.c** - Printf 重定向到 USART1（已添加到 uvprojx）
- **Core/Inc/retarget.h** - Printf 重定向头文件

### 3. HAL 驱动
- **Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c** - UART 驱动（已添加到 uvprojx）

## 修改的文件

### 1. Core/Src/main.c
**修改内容：**
```c
// 添加头文件
#include "tcp_echo_server.h"
#include "retarget.h"

// 在 main() 函数中初始化 USART1
MX_USART1_UART_Init();
printf("\r\n=== STM32F407 lwIP + RTOS TCP Echo Server ===\r\n");
printf("System starting...\r\n");

// 在 StartDefaultTask() 中初始化 TCP Echo Server
osDelay(1000);  // 等待网络初始化
tcp_echo_server_init();
```

### 2. Core/Inc/stm32f4xx_hal_conf.h
**修改内容：**
```c
// 启用 UART 模块
#define HAL_UART_MODULE_ENABLED
```

### 3. LWIP/App/lwip.c
**已有配置（无需修改）：**
- 静态IP: 192.168.1.30
- 子网掩码: 255.255.255.0
- 网关: 192.168.1.1

## Keil 项目配置（uvprojx）

### 已添加的文件（在 Application/User/Core 组）：
1. tcp_echo_server.c
2. retarget.c

### 已添加的文件（在 Drivers/STM32F4xx_HAL_Driver 组）：
1. stm32f4xx_hal_uart.c

## 需要手动配置的项目（在 Keil IDE 中）

### 调试器配置
1. 打开 **Project → Options for Target 'Desktop'**
2. 切换到 **Debug** 标签页
3. 选择你的调试器（J-LINK / ST-LINK 等）
4. 点击 **Settings** 配置：
   - Port: **SW** (SWD 模式)
   - 其他保持默认
5. 勾选 **Load Application at Startup**
6. 勾选 **Run to main()**

## 功能说明

### TCP Echo Server
- **端口**: 7 (标准 echo 端口)
- **IP**: 192.168.1.30
- **功能**: 接收客户端数据并原样回显
- **日志**: 所有网络事件通过 USART1 输出

### 串口调试
- **串口**: USART1
- **引脚**: TX=PA9, RX=PA10
- **波特率**: 115200
- **功能**: printf 输出重定向

## 测试方法

### 1. 编译下载
```
1. 打开 MDK-ARM/Desktop.uvprojx
2. 按 F7 编译
3. 按 F8 下载
```

### 2. 查看串口日志
- 连接串口工具到 PA9/PA10
- 波特率 115200
- 查看启动日志和网络事件

### 3. 测试 Echo Server
```bash
# 使用 telnet
telnet 192.168.1.30 7

# 使用 netcat
nc 192.168.1.30 7

# 使用 Python
python3 -c "import socket; s=socket.socket(); s.connect(('192.168.1.30',7)); s.send(b'Hello'); print(s.recv(1024)); s.close()"
```

## 日志输出示例

```
=== STM32F407 lwIP + RTOS TCP Echo Server ===
System starting...
[TCP Echo Server] Init OK, listening on port 7
[TCP Echo Server] Static IP: 192.168.1.30
[TCP Echo Server] New connection - Client IP: 192.168.1.100:54321
[TCP Echo Server] Received data - Length: 5 bytes, from: 192.168.1.100:54321
[TCP Echo Server] Data content: Hello
[TCP Echo Server] Data echoed
[TCP Echo Server] Client closed connection - IP: 192.168.1.100:54321
[TCP Echo Server] Connection closed
```

## 注意事项

1. **所有日志为英文** - Keil 编译器不支持 UTF-8 中文
2. **UART 模块已启用** - stm32f4xx_hal_conf.h 中已定义 HAL_UART_MODULE_ENABLED
3. **调试器需手动配置** - 根据实际硬件选择 J-Link 或 ST-Link
4. **网络配置** - 确保 PC 和板子在同一网段（192.168.1.x）
