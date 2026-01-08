# Printf 输出配置说明

## 问题：没有串口日志输出

如果编译下载后没有任何串口输出，需要在 Keil 中启用 **MicroLIB**。

## 解决方法：启用 MicroLIB

### 步骤：

1. 在 Keil 中打开项目 `MDK-ARM/Desktop.uvprojx`

2. 点击菜单 **Project → Options for Target 'Desktop'**

3. 切换到 **Target** 标签页

4. 在右侧找到 **Code Generation** 区域

5. ✅ **勾选 "Use MicroLIB"**

6. 点击 **OK** 保存

7. 重新编译（F7）和下载（F8）

## 为什么需要 MicroLIB？

- **标准 C 库**：需要实现完整的文件系统接口（`_sys_open`, `_sys_write` 等）
- **MicroLIB**：只需要实现 `fputc()` 函数即可重定向 printf

我们的代码已经实现了 `fputc()`，只需要启用 MicroLIB 即可。

## 测试

启用 MicroLIB 并重新下载后，串口应该输出：

```
*** UART TEST ***

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
...
```

## 串口连接

- **TX**: PA9
- **RX**: PA10
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无

## 如果还是没有输出

### 1. 检查硬件连接
- 确认串口线连接正确
- TX (PA9) 连接到 USB转串口的 RX
- RX (PA10) 连接到 USB转串口的 TX
- GND 连接

### 2. 检查串口工具
- 确认选择了正确的 COM 口
- 波特率设置为 115200
- 尝试复位板子

### 3. 测试 UART 硬件
代码中已经添加了直接的 UART 测试：
```c
HAL_UART_Transmit(&huart1, (uint8_t*)test_msg, strlen(test_msg), 1000);
```

如果这个也没输出，说明：
- UART 硬件初始化失败
- 引脚配置错误
- 时钟配置错误

### 4. 检查 HAL_UART_MODULE_ENABLED

确认 `Core/Inc/stm32f4xx_hal_conf.h` 中已启用：
```c
#define HAL_UART_MODULE_ENABLED
```

## 替代方案：不使用 MicroLIB

如果不想使用 MicroLIB，需要实现完整的系统调用：

```c
// 在 retarget.c 中添加
#pragma import(__use_no_semihosting)

struct __FILE {
    int handle;
};

FILE __stdout;

void _sys_exit(int x) {
    x = x;
}

int _ttywrch(int ch) {
    ch = ch;
    return ch;
}
```

但推荐直接使用 MicroLIB，更简单。
