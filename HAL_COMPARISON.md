# STM32 HAL ETH 驱动版本对比

## 问题根源

lwipandRTOS 和 stm32f407 两个项目使用了**完全不同版本**的 STM32 HAL ETH 驱动：

### 旧版 HAL (stm32f407 项目使用)

**文件**: `outsource/stm32f407/Drivers/STM32F4xx_HAL_Driver/`

**初始化参数**:
```c
g_eth_handler.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
g_eth_handler.Init.Speed = ETH_SPEED_100M;
g_eth_handler.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
g_eth_handler.Init.PhyAddress = ETHERNET_PHY_ADDRESS;
g_eth_handler.Init.MACAddr = macaddress;
g_eth_handler.Init.RxMode = ETH_RXINTERRUPT_MODE;  // ⭐ 关键：中断接收模式
g_eth_handler.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
g_eth_handler.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
```

**DMA 描述符结构** (ETH_DMADescTypeDef):
```c
typedef struct {
  __IO uint32_t Status;              // 状态字段
  uint32_t ControlBufferSize;        // 控制和缓冲区大小
  uint32_t Buffer1Addr;              // 缓冲区1地址
  uint32_t Buffer2NextDescAddr;      // 缓冲区2/下一个描述符地址
  // ... 扩展字段
} ETH_DMADescTypeDef;
```

**工作模式**:
- 传统中断模式
- HAL_ETH_Init() 自动初始化 DMA 描述符
- 使用 HAL_ETH_Start() 启动（非 _IT 版本）
- 接收流程：PHY → MAC → DMA → 中断 → HAL_ETH_IRQHandler → 用户回调

---

### 新版 HAL (lwipandRTOS 项目使用)

**文件**: `outsource/lwipandRTOS/Drivers/STM32F4xx_HAL_Driver/`

**初始化参数**:
```c
heth.Init.MACAddr = &MACAddr[0];
heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
heth.Init.TxDesc = DMATxDscrTab;      // ⭐ 需要提供描述符数组
heth.Init.RxDesc = DMARxDscrTab;      // ⭐ 需要提供描述符数组
heth.Init.RxBuffLen = 1536;
```

**DMA 描述符结构** (ETH_DMADescTypeDef):
```c
typedef struct {
  __IO uint32_t DESC0;               // 状态/控制
  __IO uint32_t DESC1;               // 控制/缓冲区大小
  __IO uint32_t DESC2;               // 缓冲区1地址
  __IO uint32_t DESC3;               // 缓冲区2/下一个描述符地址
  __IO uint32_t DESC4;               // 扩展状态
  __IO uint32_t DESC5;               // 保留
  __IO uint32_t DESC6;               // 时间戳低位
  __IO uint32_t DESC7;               // 时间戳高位
  uint32_t BackupAddr0;              // 备份地址0
  uint32_t BackupAddr1;              // 备份地址1
} ETH_DMADescTypeDef;
```

**工作模式**:
- **零拷贝 (Zero-Copy) 模式**
- 需要用户提供内存池和回调函数：
  - `HAL_ETH_RxAllocateCallback()` - 分配接收缓冲区
  - `HAL_ETH_RxLinkCallback()` - 链接 pbuf
  - `HAL_ETH_TxFreeCallback()` - 释放发送缓冲区
- 使用 HAL_ETH_Start_IT() 启动
- 描述符初始化由 `ETH_UpdateDescriptor()` 在 Start_IT 中完成

---

## 关键差异总结

| 特性 | 旧版 HAL | 新版 HAL |
|-----|---------|---------|
| 初始化参数数量 | 8个 | 5个 |
| PHY配置 | Init结构中 | 需单独配置 |
| 描述符管理 | HAL自动管理 | 用户提供+回调 |
| 内存管理 | HAL内部缓冲区 | 零拷贝+内存池 |
| 启动函数 | HAL_ETH_Start() | HAL_ETH_Start_IT() |
| 接收模式 | RxMode参数 | 固定中断模式 |
| 描述符结构 | Status/Buffer1Addr | DESC0-DESC7 |

---

## lwipandRTOS 项目的问题

### 症状
- TCP echo 第一次工作，后续失败
- `HAL_ETH_RxCpltCallback` 不被调用
- `heth.RxDescList.RxBuildDescCnt` 始终为 0

### 根本原因
新版 HAL 的 `HAL_ETH_Start_IT()` 中，`ETH_UpdateDescriptor()` 函数依赖于：
1. `heth.RxDescList.RxBuildDescCnt` 初始值应该等于 `ETH_RX_DESC_CNT`
2. 但实际上这个值在 `HAL_ETH_Init()` 后是 0
3. 导致 `ETH_UpdateDescriptor()` 不会初始化任何描述符
4. 没有描述符有 OWN 位，DMA 无法接收数据

### 当前的临时修复
在 `ethernetif.c:347-382` 手动初始化描述符：
```c
if (heth.RxDescList.RxBuildDescCnt == 0) {
    printf("[ETH] FIXING: RxBuildDescCnt is 0, manually initializing descriptors\r\n");

    for (uint32_t i = 0; i < ETH_RX_DESC_CNT; i++) {
        uint8_t *buff = NULL;
        HAL_ETH_RxAllocateCallback(&buff);
        if (buff != NULL) {
            DMARxDscrTab[i].BackupAddr0 = (uint32_t)buff;
            DMARxDscrTab[i].DESC2 = (uint32_t)buff;
            DMARxDscrTab[i].DESC1 = heth.Init.RxBuffLen | (1 << 14);
            DMARxDscrTab[i].DESC0 = (1U << 31);  // OWN bit

            if (i < ETH_RX_DESC_CNT - 1) {
                DMARxDscrTab[i].DESC3 = (uint32_t)&DMARxDscrTab[i + 1];
            } else {
                DMARxDscrTab[i].DESC3 = (uint32_t)&DMARxDscrTab[0];
            }
        }
    }
    WRITE_REG(heth.Instance->DMARDLAR, (uint32_t)&DMARxDscrTab[0]);
}
```

---

## 解决方案选项

### 选项 1: 修复新版 HAL 的使用 ⭐ 推荐
- 找出为什么 `RxBuildDescCnt` 没有被正确初始化
- 可能需要在 `HAL_ETH_Init()` 后手动设置
- 或者检查是否缺少某个初始化步骤

### 选项 2: 降级到旧版 HAL
- 替换整个 HAL ETH 驱动为旧版
- 需要修改 `ethernetif.c` 以匹配旧版 API
- 可能影响其他依赖新版 HAL 的代码

### 选项 3: 参考 STM32CubeMX 生成的代码
- 使用 STM32CubeMX 为 STM32F407 + lwIP 生成新项目
- 对比生成的 `ethernetif.c` 和当前代码
- 找出缺失的初始化步骤

---

## 下一步行动

1. ✅ 恢复正确的新版 HAL 头文件（已完成）
2. ⏳ 对比 DMA 描述符初始化流程
3. ⏳ 对比中断配置
4. ⏳ 检查 `RxBuildDescCnt` 应该在哪里被设置
5. ⏳ 测试修复后的代码

---

## 参考文件

- lwipandRTOS: `LWIP/Target/ethernetif.c:192-453`
- stm32f407: `Drivers/BSP/ETHERNET/ethernet.c:40-120`
- 新版 HAL: `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c`
