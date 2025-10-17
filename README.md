# STM32H7 HAL SD卡驱动扩展

**重要说明：本文档为中文版本，暂无英文版本。**
**Important Note: This documentation is in Chinese only, no English version available.**

## 项目概述

本项目提供了STM32H7系列微控制器的HAL库SD卡驱动扩展实现，支持UHS-I高速模式，包含完整的SD卡BSP（板级支持包）驱动实现。

## 功能特性

### 核心功能
- ✅ **SD卡初始化与状态检测**
- ✅ **多数据块读写操作（查询模式）**
- ✅ **错误诊断与处理机制**
- ✅ **性能测试与测量**
- ✅ **卡信息获取接口**

### 技术特点
- 🚀 **UHS-I高速模式支持** - 充分利用STM32H7的高速SDMMC接口
- 🔒 **参数验证机制** - 完善的参数检查和边界保护
- 📊 **详细错误诊断** - 完整的错误码解析和根因分析
- ⚡ **性能优化** - 查询模式避免FIFO溢出，支持高速传输
- 🛡️ **MISRA-C合规** - 遵循MISRA-C 2012编码规范
- 🔧 **CubeMX兼容** - 完全兼容STM32CubeMX生成的代码框架

## 文件结构

```
Drivers/BSP/
├── Inc/
│   └── sd.h          # SD卡驱动头文件
└── Src/
    └── sd.c          # SD卡驱动实现文件
```

## 快速开始

### 1. CubeMX配置要求

在使用本驱动前，请确保在STM32CubeMX中完成以下配置：

- **SDMMC1外设**：使能并配置为SD卡4位宽总线模式
- **时钟配置**：SDMMC时钟建议配置为48MHz或更高
- **DMA配置**：可选，本驱动使用查询模式
- **中断**：SDMMC1全局中断（可选）

### 2. 基本使用示例

```c
#include "sd.h"

int main(void)
{
    HAL_StatusTypeDef status;
    
    // 初始化SD卡
    status = SD_Init();
    if (status != HAL_OK) {
        // 处理初始化失败
        Error_Handler();
    }
    
    // 获取SD卡信息
    SD_CardInfoTypeDef card_info;
    status = SD_GetCardInfo(&card_info);
    if (status == HAL_OK) {
        printf("SD卡容量: %lu MB\r\n", card_info.LogBlockNbr / 2048);
    }
    
    // 数据读写示例
    uint8_t write_buffer[512] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t read_buffer[512];
    
    // 写入数据块
    status = SD_WriteBlocks(write_buffer, 0, 1, 1000);
    
    // 读取数据块
    status = SD_ReadBlocks(read_buffer, 0, 1, 1000);
    
    while (1) {
        // 主循环
    }
}
```

### 3. 性能测试

在DEBUG模式下，可以使用内置的性能测试功能：

```c
#ifdef DEBUG
    // 运行SD卡性能测试
    SD_MeasureTest();
#endif
```

测试将输出：
- 读写速度（MB/s）
- 数据传输量
- 错误统计信息

## API参考

### 初始化与状态检测

| 函数 | 说明 |
|------|------|
| `SD_Init()` | 初始化SD卡并检查状态 |
| `SD_Check()` | 检查SD卡当前状态 |
| `SD_WaitReady()` | 等待SD卡进入传输状态 |

### 数据操作

| 函数 | 说明 |
|------|------|
| `SD_WriteBlocks()` | 多块数据写入（查询模式） |
| `SD_ReadBlocks()` | 多块数据读取（查询模式） |
| `SD_EraseBlocks()` | 擦除指定数据块（宏定义） |

### 信息获取

| 函数 | 说明 |
|------|------|
| `SD_GetCardInfo()` | 获取SD卡详细信息 |
| `SD_GetStatus()` | 获取当前SD卡状态（宏定义） |

### 调试功能

| 函数 | 说明 |
|------|------|
| `SD_MeasureTest()` | 性能测试（DEBUG模式） |
| `SD_ErrorHandler()` | 错误诊断（DEBUG模式） |

## 错误处理

本驱动提供了完善的错误处理机制：

### 错误码定义
```c
#define SD_OK       HAL_OK      // 操作成功
#define SD_ERROR    HAL_ERROR   // 通用错误
#define SD_BUSY     HAL_BUSY    // SD卡忙
#define SD_TIMEOUT  HAL_TIMEOUT // 操作超时
```

### 错误诊断
在DEBUG模式下，`SD_ErrorHandler()`函数会详细分析错误原因：

- **CRC错误**：信号完整性差或时钟频率过高
- **超时错误**：卡掉线、供电不足或地址越界
- **FIFO错误**：DMA配置不当或中断干扰
- **对齐错误**：缓冲区地址未4字节对齐

## 性能优化建议

### 1. 时钟配置
- SDMMC时钟建议48MHz或更高
- 确保SD卡支持所选时钟频率

### 2. 缓冲区对齐
- 数据缓冲区必须4字节对齐
- 使用`__attribute__((aligned(4)))`声明

### 3. 块大小选择
- 标准SD卡块大小为512字节
- 多块操作比单块操作效率更高

### 4. 超时设置
- 根据操作类型设置合适的超时时间
- 写入操作通常需要更长的超时时间

## 注意事项

⚠️ **重要提醒**：

1. **CubeMX配置**：所有外设初始化必须在CubeMX中完成，禁止手动修改自动生成代码
2. **参数验证**：使用前请确保参数有效性，特别是缓冲区指针和块数量
3. **中断处理**：本驱动使用查询模式，中断模式下需要额外配置
4. **电源管理**：低功耗应用需注意SD卡电源管理
5. **调试信息**：DEBUG模式下的printf输出可能影响性能

## 版本历史

### v1.0 (2025-01-15)
- ✅ 初始版本发布
- ✅ 支持UHS-I高速模式
- ✅ 完整的错误诊断功能
- ✅ MISRA-C 2012合规
- ✅ CubeMX用户保护区兼容

## 许可证

本项目基于STMicroelectronics的BSD-3-Clause许可证发布。

## 支持与贡献

如有问题或建议，欢迎通过GitHub Issues提交。

---

**最后更新：2025年1月15日**