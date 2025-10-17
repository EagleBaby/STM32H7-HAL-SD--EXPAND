/**
  ******************************************************************************
  * @file    sd.h
  * @brief   SD卡驱动头文件 - 用户自定义区域
  * @author  STMicroelectronics
  * @date    2025-01-15
  * @version 1.0
  * @note    遵循CubeMX用户保护区规范，所有修改均在USER CODE区域内进行
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SD_H__
#define __SD_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "sdmmc.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/**
 * @defgroup SD_Error_Codes SD卡错误码定义
 * @{
 */
#define SD_OK              ((HAL_StatusTypeDef)HAL_OK)      /*!< 操作成功 */
#define SD_ERROR           ((HAL_StatusTypeDef)HAL_ERROR)    /*!< 通用错误 */
#define SD_BUSY            ((HAL_StatusTypeDef)HAL_BUSY)     /*!< SD卡忙 */
#define SD_TIMEOUT         ((HAL_StatusTypeDef)HAL_TIMEOUT) /*!< 操作超时 */
/**
 * @}
 */

/**
 * @defgroup SD_Timeout_Values SD卡超时时间定义
 * @{
 */
#define SD_TIMEOUT_DEFAULT ((uint32_t)1000U)   /*!< 默认超时时间 1秒 */
#define SD_TIMEOUT_LONG    ((uint32_t)10000U)  /*!< 长超时时间 10秒 */
/**
 * @}
 */

/**
 * @defgroup SD_Block_Size SD卡块大小定义
 * @{
 */
#define SD_BLOCK_SIZE      ((uint32_t)512U)    /*!< SD卡标准块大小 */
/**
 * @}
 */

/**
 * @brief SD卡初始化函数
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 初始化SD卡并检查卡状态
 */
HAL_StatusTypeDef SD_Init(void);

/**
 * @brief 检查SD卡状态
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 检查SD卡是否处于传输状态
 */
HAL_StatusTypeDef SD_Check(void);

/**
 * @brief 等待SD卡就绪
 * @param  Timeout: 超时时间（毫秒）
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 等待SD卡进入传输状态
 */
HAL_StatusTypeDef SD_WaitReady(uint32_t Timeout);

/**
 * @brief SD卡多块写入
 * @param  pData: 数据缓冲区指针（必须4字节对齐）
 * @param  BlockAdd: 起始块地址
 * @param  NumberOfBlocks: 块数量
 * @param  Timeout: 超时时间（毫秒）
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 使用查询模式进行多块写入，避免FIFO溢出
 */
HAL_StatusTypeDef SD_WriteBlocks(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout);

/**
 * @brief SD卡多块读取
 * @param  pData: 数据缓冲区指针（必须4字节对齐）
 * @param  BlockAdd: 起始块地址
 * @param  NumberOfBlocks: 块数量
 * @param  Timeout: 超时时间（毫秒）
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 使用查询模式进行多块读取，避免FIFO溢出
 */
HAL_StatusTypeDef SD_ReadBlocks(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout);

#ifdef DEBUG
/**
 * @brief SD卡性能测试函数
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 仅在DEBUG模式下可用，用于测试SD卡读写性能
 */
HAL_StatusTypeDef SD_MeasureTest(void);
#endif

/**
 * @brief 获取SD卡信息
 * @param  pCardInfo: 指向SD卡信息结构体的指针
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 获取SD卡的详细信息，包括容量、类型等
 */
HAL_StatusTypeDef SD_GetCardInfo(SD_CardInfoTypeDef *pCardInfo);

/* USER CODE END Private defines */

/* USER CODE BEGIN Exported types */

/**
 * @brief SD卡信息结构体
 */
typedef struct {
    uint32_t CardType;      /*!< 卡类型 */
    uint32_t CardVersion;   /*!< 卡版本 */
    uint32_t Class;         /*!< 卡类别 */
    uint32_t RelCardAdd;    /*!< 相对卡地址 */
    uint32_t BlockNbr;      /*!< 块数量 */
    uint32_t BlockSize;     /*!< 块大小 */
    uint32_t LogBlockNbr;   /*!< 逻辑块数量 */
    uint32_t LogBlockSize;  /*!< 逻辑块大小 */
} SD_CardInfoTypeDef;

/* USER CODE END Exported types */

/* USER CODE BEGIN Exported constants */

/* USER CODE END Exported constants */

/* USER CODE BEGIN Exported macro */

/**
 * @brief 获取SD卡状态
 * @retval HAL_SD_CardStateTypeDef SD卡状态
 */
#define SD_GetStatus() HAL_SD_GetCardState(&hsd1)

/**
 * @brief SD卡擦除块
 * @param  BlockAdd: 起始块地址
 * @param  NumberOfBlocks: 块数量
 * @param  Timeout: 超时时间（毫秒）
 * @retval HAL_StatusTypeDef 返回操作状态
 */
#define SD_EraseBlocks(BlockAdd, NumberOfBlocks, Timeout) \
        HAL_SD_EraseBlocks(&hsd1, (BlockAdd), (NumberOfBlocks), (Timeout))

/* USER CODE END Exported macro */

/* USER CODE BEGIN Exported functions */

/* USER CODE END Exported functions */

#ifdef __cplusplus
}
#endif

#endif /* __SD_H__ */