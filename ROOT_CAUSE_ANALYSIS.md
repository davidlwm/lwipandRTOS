# 问题根源：HAL 驱动版本不匹配

## 关键发现

### 问题 1: 使用了错误的 HAL 驱动架构

当前项目使用的 HAL ETH 驱动是 **STM32H7 架构**，但硬件是 **STM32F407**！

**证据**:
1. 使用了 `HAL_ETH_Start_IT()` 和 `ETH_UpdateDescriptor()` 等新 API
2. 使用了 `RxDescList` 结构和零拷贝机制
3. 将 `DMARPDR` 当作尾指针寄存器使用

**STM32F4 vs STM32H7 ETH DMA 差异**:

| 特性 | STM32F4 | STM32H7 |
|-----|---------|---------|
| DMA 架构 | 传统链表 | 尾指针机制 |
| DMARPDR 寄存器 | 轮询请求（写任意值） | 尾指针地址 |
| 描述符管理 | 自动轮询 | 需要设置尾指针 |
| HAL API | 旧版（DMARxDescListInit） | 新版（ETH_UpdateDescriptor） |

### 问题 2: DMARPDR 寄存器使用错误

**错误代码** (line 1292):
```c
/* Set the Tail pointer address */
WRITE_REG(heth->Instance->DMARPDR, ((uint32_t)(heth->Init.RxDesc + (tailidx))));
```

这是 STM32H7 的用法，在 STM32F4 上**不起作用**！

**STM32F4 正确用法**:
```c
/* Trigger DMA to resume reception (write any value) */
WRITE_REG(heth->Instance->DMARPDR, 0);
```

在 STM32F4 中，DMARPDR 是 **Receive Poll Demand Register**，写入任意值会触发 DMA 检查描述符链表。

---

## 已实施的修复

### 修复 1: 更正 DMARPDR 寄存器用法

**文件**: `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c:1281`

**修改**:
```c
if (heth->RxDescList.RxBuildDescCnt != desccount)
{
  /* STM32F4: DMARPDR is Receive Poll Demand, not tail pointer */
  printf("[ETH_UpdateDesc] Triggering DMA poll demand\r\n");

  __DMB();

  /* Trigger DMA to resume reception (write any value) */
  WRITE_REG(heth->Instance->DMARPDR, 0);

  heth->RxDescList.RxBuildDescIdx = descidx;
  heth->RxDescList.RxBuildDescCnt = desccount;
}
```

---

## 测试步骤

1. **重新编译代码**
2. **烧录到板子**
3. **复位并观察日志**，应该看到：
   ```
   [ETH_UpdateDesc] Triggering DMA poll demand
   ```
4. **发送测试数据**:
   ```bash
   echo "test" | nc 192.168.1.30 8080
   ```
5. **观察是否有 RX 中断**

---

## 如果仍然不工作

### 方案 A: 继续修复当前 HAL 驱动 ⚠️ 复杂

需要检查并修复所有 STM32H7 特有的代码，包括：
- `HAL_ETH_ReadData()` 中的描述符处理
- `HAL_ETH_BuildRxDescriptors()` 的实现
- 中断处理流程

**风险**: 可能还有其他不兼容的地方。

### 方案 B: 使用 STM32F4 旧版 HAL 驱动 ⭐ 推荐

从你的 `stm32f407` 参考项目复制旧版 HAL 驱动：

**需要替换的文件**:
1. `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c`
2. `Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_eth.h`

**需要修改的文件**:
1. `LWIP/Target/ethernetif.c` - 适配旧版 API

**旧版 API 差异**:
```c
// 旧版初始化
heth.Init.RxMode = ETH_RXINTERRUPT_MODE;
HAL_ETH_Init(&heth);
HAL_ETH_DMARxDescListInit(&heth, DMARxDscrTab, RxBuff, ETH_RXBUFNB);
HAL_ETH_DMATxDescListInit(&heth, DMATxDscrTab, TxBuff, ETH_TXBUFNB);
HAL_ETH_Start(&heth);  // 不是 _IT 版本

// 旧版接收
HAL_ETH_GetReceivedFrame_IT(&heth);
```

### 方案 C: 使用 STM32CubeMX 重新生成 ⭐⭐ 最推荐

1. 打开 STM32CubeMX
2. 选择 STM32F407ZGT6
3. 配置 ETH + lwIP + FreeRTOS
4. 生成代码
5. 对比生成的 `ethernetif.c` 和当前代码
6. 采用生成的代码

**优点**:
- 保证 HAL 驱动版本正确
- 配置经过验证
- 有官方支持

---

## YT8512C PHY 配置

你提到使用的是 YT8512C PHY 芯片，需要确认：

### 1. PHY 地址
```c
#define ETHERNET_PHY_ADDRESS  0  // 或 1，取决于硬件配置
```

### 2. PHY 驱动
当前代码使用 LAN8742 驱动，需要替换为 YT8512C 驱动：

**文件**: `LWIP/Target/ethernetif.c:122`
```c
// 当前（错误）
lan8742_Object_t LAN8742;
lan8742_IOCtx_t  LAN8742_IOCtx = {...};

// 应该改为
yt8512c_Object_t YT8512C;
yt8512c_IOCtx_t  YT8512C_IOCtx = {...};
```

### 3. PHY 初始化
```c
// 注册 PHY 驱动
YT8512C_RegisterBusIO(&YT8512C, &YT8512C_IOCtx);

// 初始化 PHY
YT8512C_Init(&YT8512C);

// 获取链路状态
PHYLinkState = YT8512C_GetLinkState(&YT8512C);
```

**注意**: 你的 `stm32f407` 参考项目中应该有 YT8512C 的驱动代码，可以直接复制过来。

---

## 推荐的行动计划

### 立即测试（5分钟）
1. 编译当前修复（DMARPDR 修复）
2. 测试是否有 RX 中断
3. 如果有效，问题解决！

### 如果不工作（1-2小时）
1. 从 `stm32f407` 项目复制旧版 HAL 驱动
2. 修改 `ethernetif.c` 适配旧版 API
3. 复制 YT8512C PHY 驱动
4. 测试

### 长期方案（2-3小时）
1. 使用 STM32CubeMX 重新生成项目
2. 迁移应用代码
3. 彻底解决兼容性问题

---

## 参考资料

- STM32F4 参考手册 RM0090: Section 33 (Ethernet)
- STM32H7 参考手册 RM0433: Section 55 (Ethernet)
- AN4838: Managing memory protection unit in STM32 MCUs
- 你的工作项目: `outsource/stm32f407/` (已验证可用)

---

## 下一步

请先测试 DMARPDR 修复，然后告诉我结果。如果仍然不工作，我们可以：
1. 继续调试当前 HAL
2. 或者切换到旧版 HAL（更快更可靠）

你倾向于哪种方案？
