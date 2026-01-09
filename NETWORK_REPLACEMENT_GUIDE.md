# STM32F407网络功能替换完整指南

## 执行日期
2026-01-09

## 替换目标
将旧版STM32F407项目的网络功能替换到当前lwipandRTOS项目中，以解决网络连接问题。

## 已完成的修改

### 1. 文件复制
已成功复制以下文件到lwipandRTOS项目：

```
Drivers/BSP/ETHERNET/
├── ethernet.c    (已修改)
└── ethernet.h    (已修改)

Middlewares/lwip/arch/
├── ethernetif.c
├── ethernetif.h
├── lwip_comm.c   (已修改)
├── lwip_comm.h   (已修改)
└── lwipopts.h

Middlewares/lwip/lwip_app/
├── lwip_demo.c
└── lwip_demo.h
```

### 2. 代码修改详情

#### ethernet.c
- ✅ 移除 `#include "./SYSTEM/sys/sys.h"`
- ✅ 移除 `#include "./SYSTEM/delay/delay.h"`
- ✅ 移除 `#include "./MALLOC/malloc.h"`
- ✅ 添加 `#include "stm32f4xx_hal.h"`
- ✅ 添加 `#include <stdlib.h>` 和 `#include <string.h>`
- ✅ 将 `delay_ms(50)` 替换为 `HAL_Delay(50)`
- ✅ 将 `mymalloc(SRAMIN, ...)` 替换为 `malloc(...)`
- ✅ 将 `myfree(SRAMIN, ...)` 替换为 `free(...)`
- ✅ 添加NULL检查和memset初始化

#### ethernet.h
- ✅ 将 `#include "./SYSTEM/sys/sys.h"` 替换为 `#include "stm32f4xx_hal.h"`

#### lwip_comm.h
- ✅ 将 `#include "./BSP/ETHERNET/ethernet.h"` 替换为 `#include "ethernet.h"`

#### lwip_comm.c
- ✅ 移除 `#include "./MALLOC/malloc.h"`
- ✅ 移除 `#include "./SYSTEM/delay/delay.h"`
- ✅ 移除 `#include "./SYSTEM/usart/usart.h"`
- ✅ 移除 `#include "debug_print.h"`
- ✅ 添加 `#include <string.h>`
- ✅ 将所有 `my_printf` 替换为 `printf`
- ✅ 将所有 `smy_printf` 替换为 `sprintf`

### 3. 备份位置
- 原新版网络文件备份：`lwipandRTOS/backup_new_network_20260109_142701/`

## 下一步工作

### 步骤1：更新项目头文件路径（IDE配置）

需要在你的IDE（Keil MDK/STM32CubeIDE等）中添加以下头文件搜索路径：

```
Drivers/BSP/ETHERNET
Middlewares/lwip/arch
Middlewares/lwip/lwip_app
```

**Keil MDK操作：**
1. 右键项目 → Options → C/C++ → Include Paths
2. 添加上述三个路径

**STM32CubeIDE操作：**
1. 右键项目 → Properties → C/C++ Build → Settings
2. MCU GCC Compiler → Include paths → 添加上述三个路径

### 步骤2：添加源文件到编译列表

需要在IDE中添加以下源文件到编译列表：

```
Drivers/BSP/ETHERNET/ethernet.c
Middlewares/lwip/arch/ethernetif.c
Middlewares/lwip/arch/lwip_comm.c
Middlewares/lwip/lwip_app/lwip_demo.c
```

**Keil MDK操作：**
1. 在Project窗口右键 → Add Group
2. 创建新分组（如：ETHERNET_DRIVER）
3. 右键新分组 → Add Existing Files
4. 浏览并添加上述.c文件

### 步骤3：修改main.c

需要将main.c中的网络初始化代码替换为旧版方式：

```c
// 在main.c头部添加
#include "lwip_comm.h"
#include "ethernetif.h"

// 在StartDefaultTask函数中，替换 MX_LWIP_Init() 为：
void StartDefaultTask(void *argument)
{
  printf("\r\n[Task] Default task started\r\n");
  printf("[Task] Initializing lwIP stack (old version)...\r\n");

  // 使用旧版网络初始化
  if (lwip_comm_init() == 0) {
    printf("[Task] lwIP initialized successfully\r\n");
  } else {
    printf("[Task] lwIP initialization FAILED!\r\n");
    Error_Handler();
  }

  /* Wait for network initialization */
  printf("[Task] Waiting for network link (2 seconds)...\r\n");
  osDelay(2000);

  // TCP Echo服务器或其他应用可以在这里初始化
  // tcp_echo_server_init();

  printf("[Task] System ready\r\n");
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
}
```

### 步骤4：禁用或移除旧版网络文件

为了避免冲突，需要禁用或移除以下文件：

```
LWIP/App/lwip.c
LWIP/Target/ethernetif.c
Drivers/BSP/Components/lan8742/lan8742.c
```

**Keil MDK操作：**
1. 右键文件 → Options
2. 勾选 "Exclude from Build"
3. 点击OK

### 步骤5：PHY地址配置

需要在项目配置中添加PHY地址定义。在ethernet.h或项目配置中添加：

```c
// 以太网PHY芯片地址
#define ETHERNET_PHY_ADDRESS            0x00  // LAN8720/LAN8742默认地址

// PHY类型选择
#define PHY_TYPE                        LAN8720  // 或 LAN8742

// PHY状态寄存器定义
#define PHY_BSR                         0x1F  // Basic Status Register
#define PHY_LINKED_STATUS               0x0004  // Link Status

// 以太网缓冲区大小
#define ETH_RX_BUF_SIZE                 1536  // 接收缓冲区大小
#define ETH_TX_BUF_SIZE                 1536  // 发送缓冲区大小

// 以太网描述符数量
#define ETH_RXBUFNB                     4     // 接收描述符数量
#define ETH_TXBUFNB                     4     // 发送描述符数量
```

### 步骤6：编译项目

1. 清理项目（Clean）
2. 重新编译（Rebuild All）
3. 检查并修复编译错误

常见编译问题：
- `undefined reference to 'printf'` → 需要添加stdio.h并重定向printf
- `multiple definition` → 检查是否有重复的源文件
- `cannot open source input file` → 检查头文件路径配置

### 步骤7：测试网络功能

1. 下载程序到STM32F407
2. 连接串口，观察输出
3. 使用ping测试：`ping 192.168.1.30`
4. 测试TCP连接

## 预期串口输出

正常情况下，应该看到类似输出：

```
========================================
  STM32F407 lwIP + RTOS
========================================
System starting...
USART1 initialized: TX=PA9, RX=PA10, 115200bps

[Task] Default task started
[Task] Initializing lwIP stack (old version)...
enMAC地址为:................184.174.29.0.1.0
静态IP地址........................192.168.1.30
LWIP_LINK_ON
[Task] lwIP initialized successfully
[Task] System ready
========================================
```

## 故障排查

### 问题1：编译错误 - 找不到头文件
**解决：** 检查IDE的头文件搜索路径配置

### 问题2：链接错误 - multiple definition
**解决：** 确保禁用了旧版的ethernetif.c和lan8742.c

### 问题3：网络无法ping通
**解决：** 
- 检查网线连接
- 检查IP地址是否正确
- 检查PHY芯片是否正常工作
- 使用示波器检查PHY晶振

### 问题4：串口无输出
**解决：**
- 检查USART1初始化
- 检查printf重定向
- 检查串口线连接

## 文件对照表

| 功能 | 旧版（已复制） | 新版（已备份） |
|-----|------------|------------|
| BSP驱动 | ethernet.c/h | (无对应) |
| lwIP接口 | ethernetif.c/h | LWIP/Target/ethernetif.c/h |
| lwIP通信 | lwip_comm.c/h | LWIP/App/lwip.c |
| PHY驱动 | (集成在ethernet.c) | lan8742.c/h |
| lwIP配置 | lwipopts.h | LWIP/Target/lwipopts.h |

## 关键差异

### 架构差异
- **旧版**：BSP驱动直接控制PHY，简单直接
- **新版**：使用独立的PHY驱动层，结构更复杂

### 内存管理
- **旧版**：使用自定义mymalloc/myfree → 已适配为标准malloc/free
- **新版**：使用lwIP内存池

### 调试输出
- **旧版**：使用my_printf → 已适配为printf
- **新版**：使用printf

## 成功标志

网络替换成功的标志：
1. ✅ 编译无错误
2. ✅ 串口输出正确的IP地址
3. ✅ 可以ping通STM32的IP地址
4. ✅ 可以建立TCP连接
5. ✅ 长时间运行稳定

## 回退方法

如果需要回退到新版网络：
1. 删除或重命名新添加的文件
2. 从备份目录恢复文件
3. 重新启用旧版网络文件的编译
4. 恢复main.c的原始代码

## 技术支持

如遇到问题，请检查：
1. STM32F407开发板型号是否正确
2. PHY芯片型号（LAN8720或LAN8742）
3. 网络环境和IP配置
4. 串口波特率设置（115200）

## 更新历史
- 2026-01-09: 初始版本，完成网络文件替换和依赖适配

