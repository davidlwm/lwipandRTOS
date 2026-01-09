# RX Callback 不触发问题诊断

## 问题描述

**症状**：只有当 `HAL_ETH_RxCpltCallback` 被调用时，TCP 才正常工作。有时候这个回调不被触发。

## 可能的原因

### 原因 1: RX 描述符耗尽 ⭐ 最可能

**现象**：
- 所有 RX 描述符都被占用（没有 OWN 位）
- DMA 无法接收新数据包
- 不会触发 RX 中断

**根本原因**：
- `HAL_ETH_ReadData()` 读取数据包后，需要调用 `HAL_ETH_BuildRxDescriptors()` 来释放描述符
- 如果描述符没有被正确释放，最终会耗尽所有描述符

**检查方法**：
观察日志中的 `RxBuildDescCnt` 值：
```
[ETH IRQ] RxBuildDescCnt=0, RxDescIdx=2  ← 正常，有描述符可用
[ETH IRQ] RxBuildDescCnt=4, RxDescIdx=0  ← 警告！所有描述符都需要重建
```

如果 `RxBuildDescCnt` 持续增长到 `ETH_RX_DESC_CNT` (4)，说明描述符没有被释放。

---

### 原因 2: 内存池耗尽

**现象**：
- `HAL_ETH_RxAllocateCallback()` 返回 NULL
- `RxAllocStatus = RX_ALLOC_ERROR`
- `low_level_input()` 不会调用 `HAL_ETH_ReadData()`

**根本原因**：
- RX_POOL 大小为 12 个缓冲区
- 如果 lwIP 没有及时释放 pbuf，内存池会耗尽

**检查方法**：
观察日志：
```
[ETH_ALLOC] #13: FAILED - RX_POOL exhausted!
[LowLevelInput] #50: RxAllocStatus ERROR!
```

---

### 原因 3: 中断被禁用或优先级问题

**现象**：
- ETH 中断被禁用
- 或者被更高优先级的中断阻塞

**检查方法**：
在 `stm32f4xx_it.c` 中添加计数器：
```c
void ETH_IRQHandler(void)
{
  static uint32_t irq_total = 0;
  irq_total++;

  if (irq_total % 100 == 0) {
    printf("[ETH_IRQ] Total interrupts: %lu\r\n", irq_total);
  }

  HAL_ETH_IRQHandler(&heth);
}
```

如果 `irq_total` 不增长，说明中断没有触发。

---

### 原因 4: DMA 停止接收

**现象**：
- DMA 接收被意外停止
- `DMAOMR` 寄存器的 SR 位被清除

**检查方法**：
定期检查 DMA 状态：
```c
uint32_t dmaomr = heth.Instance->DMAOMR;
uint32_t dmasr = heth.Instance->DMASR;

printf("[DMA Status] DMAOMR=0x%08lX (SR bit=%d), DMASR=0x%08lX\r\n",
       dmaomr, (dmaomr & ETH_DMAOMR_SR) ? 1 : 0, dmasr);
```

如果 SR 位为 0，说明 DMA 接收已停止。

---

## 诊断流程

### 步骤 1: 收集完整日志

运行测试并收集以下日志：

1. **启动时的初始化日志**：
   ```
   [ETH] Starting ETH DMA...
   [ETH] HAL_ETH_Start_IT result: 0 (0=OK)
   [ETH] After Start: RxBuildDescCnt=0
   ```

2. **第一次 TCP 连接成功时的日志**：
   ```
   [ETH IRQ] RX callback #1
   [ETH IRQ] RxBuildDescCnt=0, RxDescIdx=0
   [EthInput] #1: Semaphore acquired
   [LowLevelInput] #1: SUCCESS, pbuf=0x20001234, len=60
   [EthInput] #1: Got packet 1, len=60
   [EthInput] #1: Processed 1 packets, RxBuildDescCnt=1
   ```

3. **第二次 TCP 连接失败时的日志**：
   - 是否有 `[ETH IRQ]` 日志？
   - 是否有 `[EthInput]` 日志？
   - `RxBuildDescCnt` 的值是多少？

### 步骤 2: 分析日志模式

#### 模式 A: 有中断但没有数据包
```
[ETH IRQ] RX callback #10
[ETH IRQ] RxBuildDescCnt=0, RxDescIdx=3
[EthInput] #10: Semaphore acquired
[LowLevelInput] #10: No packet (OK)  ← HAL_ETH_ReadData 返回 NULL
[EthInput] #10: Processed 0 packets, RxBuildDescCnt=0
```

**诊断**：虚假中断或 DMA 状态异常。

---

#### 模式 B: RxBuildDescCnt 持续增长
```
[ETH IRQ] RX callback #1, RxBuildDescCnt=0
[ETH IRQ] RX callback #2, RxBuildDescCnt=1
[ETH IRQ] RX callback #3, RxBuildDescCnt=2
[ETH IRQ] RX callback #4, RxBuildDescCnt=3
[ETH IRQ] RX callback #5, RxBuildDescCnt=4  ← 所有描述符都需要重建
(no more callbacks)  ← 描述符耗尽，无法接收
```

**诊断**：描述符没有被正确释放。

**解决方法**：检查 `HAL_ETH_ReadData()` 后是否调用了 `HAL_ETH_BuildRxDescriptors()`。

---

#### 模式 C: 内存池耗尽
```
[ETH_ALLOC] #1-12: SUCCESS
[ETH_ALLOC] #13: FAILED - RX_POOL exhausted!
[LowLevelInput] #50: RxAllocStatus ERROR!
(no more RX callbacks)
```

**诊断**：lwIP 没有释放 pbuf，或者 RX_POOL 太小。

**解决方法**：
1. 增加 `ETH_RX_BUFFER_CNT` (当前是 12)
2. 检查 lwIP 配置，确保 pbuf 被正确释放

---

#### 模式 D: 完全没有中断
```
(启动后没有任何 [ETH IRQ] 日志)
```

**诊断**：
1. 中断没有被使能
2. PHY 链路断开
3. MAC/DMA 配置错误

**解决方法**：
1. 检查 `NVIC` 配置
2. 检查 `DMAIER` 寄存器
3. 检查 PHY 链路状态

---

## 修复方案

### 方案 1: 确保描述符被正确释放

在 `HAL_ETH_ReadData()` 之后，HAL 库应该自动调用 `ETH_UpdateDescriptor()` 来重建描述符。

检查 `HAL_ETH_ReadData()` 的实现：

```c
HAL_StatusTypeDef HAL_ETH_ReadData(ETH_HandleTypeDef *heth, void **pAppBuff)
{
  // ... 读取数据 ...

  // 重建描述符
  ETH_UpdateDescriptor(heth);  // ← 这一步很关键！

  return HAL_OK;
}
```

如果没有调用，需要手动调用：
```c
HAL_ETH_ReadData(&heth, (void **)&p);
// 手动重建描述符（如果 HAL 没有自动做）
// ETH_UpdateDescriptor(&heth);  // 但这是 static 函数，无法直接调用
```

---

### 方案 2: 增加 RX 缓冲区数量

修改 `ethernetif.c:95`：
```c
#define ETH_RX_BUFFER_CNT             24U  // 从 12 增加到 24
```

---

### 方案 3: 添加看门狗检测

在 `ethernetif_input` 中添加超时检测：
```c
void ethernetif_input(void* argument)
{
  for( ;; )
  {
    if (osSemaphoreAcquire(RxPktSemaphore, 5000) == osOK)  // 5秒超时
    {
      // 正常处理
    }
    else
    {
      // 超时，检查 DMA 状态
      printf("[EthInput] TIMEOUT! Checking DMA status...\r\n");
      printf("[EthInput] DMAOMR=0x%08lX, DMASR=0x%08lX\r\n",
             heth.Instance->DMAOMR, heth.Instance->DMASR);
      printf("[EthInput] RxBuildDescCnt=%lu\r\n",
             (unsigned long)heth.RxDescList.RxBuildDescCnt);

      // 如果 RxBuildDescCnt > 0，尝试手动重建描述符
      if (heth.RxDescList.RxBuildDescCnt > 0) {
        printf("[EthInput] Attempting to rebuild descriptors...\r\n");
        // 调用 HAL_ETH_BuildRxDescriptors() 或类似函数
      }
    }
  }
}
```

---

## 测试步骤

1. **编译并烧录新代码**（已添加详细日志）

2. **第一次测试**：
   ```bash
   echo "test1" | nc 192.168.1.30 8080
   ```
   观察日志，记录：
   - 有多少个 `[ETH IRQ]` 回调？
   - `RxBuildDescCnt` 的变化？
   - 是否成功接收和回显？

3. **第二次测试**（立即执行）：
   ```bash
   echo "test2" | nc 192.168.1.30 8080
   ```
   观察日志，记录：
   - 是否有 `[ETH IRQ]` 回调？
   - 如果没有，`RxBuildDescCnt` 是多少？
   - 是否有 `[ETH_ALLOC] FAILED` 日志？

4. **等待 10 秒后第三次测试**：
   ```bash
   echo "test3" | nc 192.168.1.30 8080
   ```
   观察是否恢复正常。

---

## 预期结果

### 正常情况
```
[ETH IRQ] RX callback #1, RxBuildDescCnt=0, RxDescIdx=0
[EthInput] #1: Semaphore acquired
[LowLevelInput] #1: SUCCESS, pbuf=0x20001234, len=60
[EthInput] #1: Got packet 1, len=60
[EthInput] #1: Processed 1 packets, RxBuildDescCnt=0  ← 描述符已释放

[ETH IRQ] RX callback #2, RxBuildDescCnt=0, RxDescIdx=1
[EthInput] #2: Semaphore acquired
[LowLevelInput] #2: SUCCESS, pbuf=0x20001456, len=60
[EthInput] #2: Got packet 1, len=60
[EthInput] #2: Processed 1 packets, RxBuildDescCnt=0  ← 描述符已释放
```

### 异常情况（描述符耗尽）
```
[ETH IRQ] RX callback #1, RxBuildDescCnt=0, RxDescIdx=0
[EthInput] #1: Processed 1 packets, RxBuildDescCnt=1  ← 描述符未释放！

[ETH IRQ] RX callback #2, RxBuildDescCnt=1, RxDescIdx=1
[EthInput] #2: Processed 1 packets, RxBuildDescCnt=2  ← 累积！

[ETH IRQ] RX callback #3, RxBuildDescCnt=2, RxDescIdx=2
[EthInput] #3: Processed 1 packets, RxBuildDescCnt=3

[ETH IRQ] RX callback #4, RxBuildDescCnt=3, RxDescIdx=3
[EthInput] #4: Processed 1 packets, RxBuildDescCnt=4  ← 所有描述符耗尽

(no more callbacks - DMA 无法接收)
```

---

## 下一步

请运行测试并提供完整的日志输出，特别关注：
1. `RxBuildDescCnt` 的变化趋势
2. 是否有内存分配失败
3. 第二次连接失败时的具体日志

根据日志，我们可以确定具体的失败原因并实施相应的修复。
