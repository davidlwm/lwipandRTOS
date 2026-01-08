# 编译和下载检查清单

## 问题：代码更新了但运行的还是旧代码

### 原因分析
1. Keil 使用了缓存的 .o 文件
2. 没有完全重新编译
3. 下载的是旧的 .hex/.bin 文件

## 解决步骤

### 方法 1：完全清理重新编译（推荐）

1. **清理项目**
   - 在 Keil 中点击菜单：**Project → Clean Targets**
   - 或按快捷键（如果有配置）

2. **重新编译**
   - 点击菜单：**Project → Rebuild all target files**
   - 或按 **F7** 编译

3. **检查编译输出**
   - 确认看到所有文件都被重新编译
   - 查看是否有 `main.c`, `lwip.c`, `tcp_echo_server.c` 被编译

4. **下载**
   - 按 **F8** 下载到板子

5. **复位板子**
   - 按复位按钮或重新上电

### 方法 2：删除编译输出目录

1. **关闭 Keil**

2. **删除编译输出**
   ```bash
   # 在项目目录下
   rm -rf MDK-ARM/Desktop/
   ```

3. **重新打开 Keil**

4. **编译下载**

### 方法 3：检查文件时间戳

在 Keil 的 Build Output 窗口中，应该看到：

```
compiling main.c...
compiling lwip.c...
compiling tcp_echo_server.c...
compiling retarget.c...
...
linking...
Program Size: Code=XXXXX RO-data=XXXX RW-data=XXX ZI-data=XXXXX
FromELF: creating hex file...
"Desktop\Desktop.axf" - 0 Error(s), 0 Warning(s).
```

如果看到 `"Desktop\Desktop.axf" - 0 Error(s), 0 Warning(s).` 但没有看到 `compiling main.c...`，说明 Keil 认为文件没有改变。

## 当前代码应该输出的日志

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
[lwIP] MX_LWIP_Init() returning...          <-- 新增
[Task] lwIP initialized successfully         <-- 新增
[Task] Waiting for network link (2 seconds)... <-- 新增
```

## 如果还是看不到新日志

### 检查 1：确认文件被修改
```bash
# 检查文件内容
grep "MX_LWIP_Init() returning" /home/david/shareDir/branch/outsource/lwipandRTOS/LWIP/App/lwip.c
grep "lwIP initialized successfully" /home/david/shareDir/branch/outsource/lwipandRTOS/Core/Src/main.c
```

应该都能找到这些字符串。

### 检查 2：确认 Keil 项目路径正确
- 确认打开的是正确的项目文件
- 路径：`/home/david/shareDir/branch/outsource/lwipandRTOS/MDK-ARM/Desktop.uvprojx`

### 检查 3：查看编译时间
在 Keil 的 Build Output 中查看：
```
Build Time Elapsed:  00:00:XX
```

如果时间很短（< 5 秒），可能没有重新编译所有文件。

### 检查 4：手动触发重新编译
右键点击 `main.c` → **Compile**
右键点击 `lwip.c` → **Compile**
右键点击 `tcp_echo_server.c` → **Compile**

然后再 **Build** 整个项目。

## 强制 Keil 重新编译的方法

### 方法 A：修改文件触发重新编译
在文件中添加一个空格，然后保存，Keil 会检测到文件改变。

### 方法 B：使用 Touch 命令更新时间戳
```bash
touch /home/david/shareDir/branch/outsource/lwipandRTOS/Core/Src/main.c
touch /home/david/shareDir/branch/outsource/lwipandRTOS/LWIP/App/lwip.c
touch /home/david/shareDir/branch/outsource/lwipandRTOS/Core/Src/tcp_echo_server.c
```

### 方法 C：在 Keil 中设置
**Project → Options for Target → C/C++ → Optimization**
- 临时改变优化级别（例如从 -O2 改为 -O1）
- 这会强制重新编译所有文件
- 编译完成后可以改回来

## 验证新代码已下载

如果看到以下日志，说明新代码已经运行：
- ✅ `[lwIP] MX_LWIP_Init() returning...`
- ✅ `[Task] lwIP initialized successfully`
- ✅ `[Task] Waiting for network link (2 seconds)...`

如果还是只看到：
```
[lwIP] Network interface is UP
[lwIP] Ethernet link thread created
```

说明：
1. 代码没有被重新编译，或
2. 程序卡在 `osThreadNew(ethernet_link_thread, ...)` 之后
