# 栈溢出问题解决方案

## 问题诊断

### 症状
程序在 `netif_add()` 调用时卡住：
```
[lwIP] Adding network interface...
```
之后没有任何输出。

### 根本原因
**任务栈太小导致栈溢出**

原始配置：
- `defaultTask` 栈大小：128 * 4 = **512 字节**
- `TCPIP_THREAD` 栈大小：**1024 字节**

这对于 lwIP + 以太网初始化来说远远不够！

## 解决方案

### 已修改的配置

#### 1. 增加 defaultTask 栈大小
**文件**: `Core/Src/main.c`

```c
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2048 * 4,  // 8KB (原来是 128*4 = 512字节)
  .priority = (osPriority_t) osPriorityNormal,
};
```

#### 2. 增加 TCPIP 线程栈大小
**文件**: `LWIP/Target/lwipopts.h`

```c
#define TCPIP_THREAD_STACKSIZE 2048  // 原来是 1024
```

## 为什么需要这么大的栈？

### lwIP 栈使用情况
- `ethernetif_init()` - 以太网硬件初始化
- PHY 芯片初始化和配置
- DMA 缓冲区分配
- 网络接口结构体
- printf 缓冲区
- 函数调用栈帧

### 推荐的栈大小

| 任务 | 最小栈 | 推荐栈 | 说明 |
|-----|--------|--------|------|
| defaultTask | 4KB | 8KB | 运行 lwIP 初始化和应用代码 |
| TCPIP Thread | 1.5KB | 2KB | lwIP 协议栈处理 |
| Ethernet Link | 512B | 1KB | 以太网链路监控 |

## 重新编译和测试

### 步骤
1. **Rebuild all** - 完全重新编译
2. **下载到板子**
3. **复位**
4. **观察日志**

### 预期的完整日志

```
========================================
  STM32F407 lwIP + RTOS TCP Echo Server
========================================
System starting...
USART1 initialized: TX=PA9, RX=PA10, 115200bps

[Task] Default task started
[Task] Initializing lwIP stack...
[lwIP] === MX_LWIP_Init() START ===

[lwIP] Network Configuration:
[lwIP]   IP Address: 192.168.1.30
[lwIP]   Netmask:    255.255.255.0
[lwIP]   Gateway:    192.168.1.1
[lwIP] Calling tcpip_init()...
[lwIP] tcpip_init() completed
[lwIP] IP addresses configured
[lwIP] Adding network interface...
[lwIP] Network interface added              <-- 应该能看到这行了！
[lwIP] Setting default interface...
[lwIP] Bringing interface up...
[lwIP] Setting link callback...
[lwIP] Creating Ethernet link thread...
[lwIP] Ethernet link thread created
[lwIP] MX_LWIP_Init() returning...
[Task] lwIP initialized successfully
[Task] Waiting for network link (2 seconds)...

[lwIP] *** Ethernet Link UP ***
[lwIP] Network is ready to accept connections

[Task] Delay complete, checking network status...
[Task] Starting TCP Echo Server...
[TCP Echo Server] Starting initialization...
[TCP Echo Server] TCP PCB created: 0xXXXXXXXX
[TCP Echo Server] Binding to port 7...
[TCP Echo Server] Bind result: 0

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
```

## 如何检测栈溢出

### FreeRTOS 栈检查
在 `FreeRTOSConfig.h` 中启用栈检查：

```c
#define configCHECK_FOR_STACK_OVERFLOW 2
```

然后实现回调函数：

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("STACK OVERFLOW in task: %s\r\n", pcTaskName);
    while(1);  // 停在这里
}
```

### 运行时检查剩余栈
```c
UBaseType_t uxHighWaterMark;
uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
printf("Stack remaining: %lu bytes\r\n", uxHighWaterMark * 4);
```

## 内存使用估算

### 增加栈后的内存使用
- defaultTask: 8KB
- TCPIP Thread: 2KB
- Ethernet Link: 1KB (INTERFACE_THREAD_STACK_SIZE)
- 其他系统任务: ~2KB
- **总计**: ~13KB

STM32F407ZGT6 有 **192KB RAM**，使用 13KB 完全没问题。

## 其他可能的优化

如果内存紧张，可以：

1. **减少 TCP 缓冲区**（lwipopts.h）
   ```c
   #define TCP_MSS 536
   #define TCP_SND_BUF (2*TCP_MSS)
   ```

2. **减少 PBUF 数量**
   ```c
   #define MEMP_NUM_PBUF 8
   ```

3. **使用更小的 MEM_SIZE**
   ```c
   #define MEM_SIZE (5*1024)
   ```

但对于 STM32F407，不需要这些优化。
