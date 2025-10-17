/**
  ******************************************************************************
  * @file    sd.c
  * @brief   SD卡驱动实现文件 - 用户自定义区域
  * @author  STMicroelectronics
  * @date    2025-10-15
  * @version 1.0
  * @note    遵循CubeMX用户保护区规范，所有修改均在USER CODE区域内进行
  * @note    符合MISRA-C 2012规范，支持调试模式下的详细诊断
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

/* Includes ------------------------------------------------------------------*/
#include "sd.h"

/* USER CODE BEGIN 0 */
#include <string.h>  /* MISRA-C 要求显式包含 */

#ifdef DEBUG
#include <stdio.h>   /* 仅在DEBUG模式下包含 */
#endif

#ifdef DEBUG
static void SD_ErrorHandler(const char* operation);  /* 前向声明 */
#endif

static uint32_t tickstart;  /* 超时计数器 */

/* USER CODE BEGIN 1 */

/**
  * @brief  SD卡初始化
  * @retval HAL_StatusTypeDef 返回操作状态
  * @note   等待SD卡就绪并检查卡状态，超时时间1秒
  */
HAL_StatusTypeDef SD_Init(void)
{
  HAL_SD_CardStateTypeDef card_state;
  HAL_StatusTypeDef status = HAL_OK;
  
#ifdef DEBUG
  printf("[SD] SD卡初始化...\r\n");
  printf("[SD] hsd1.State = %d, hsd1.ErrorCode = 0x%08lX\r\n", hsd1.State, hsd1.ErrorCode);
#endif
  
  /* 等待SD卡就绪（1秒超时）*/
  tickstart = HAL_GetTick();
  while ((HAL_GetTick() - tickstart) < SD_TIMEOUT_DEFAULT)
  {
    if (hsd1.State == HAL_SD_STATE_READY)
    {
      break;
    }
  }
  
  if (hsd1.State != HAL_SD_STATE_READY)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] SD卡未就绪，状态: %d\r\n", hsd1.State);
#endif
    status = HAL_ERROR;
  }
  else
  {
    /* 检查卡状态 */
    card_state = HAL_SD_GetCardState(&hsd1);
    if (card_state != HAL_SD_CARD_TRANSFER)
    {
#ifdef DEBUG
      printf("[SD] SD卡状态异常: %lu\r\n", card_state);
#endif
      status = HAL_ERROR;
    }
  }
  
#ifdef DEBUG
  if (status == HAL_OK)
  {
    SD_CardInfoTypeDef card_info;
    HAL_StatusTypeDef info_status;
    
    info_status = SD_GetCardInfo(&card_info);
    if (info_status == HAL_OK)
    {
      uint32_t total_mb = card_info.LogBlockNbr / 2048U;  /* 总MB数 */
      uint32_t gb_int = total_mb / 1024U;                 /* GB整数部分 */
      uint32_t gb_decimal = ((total_mb % 1024U) * 10U) / 1024U; /* GB1位小数 */
      
      printf("[SD] [PASS] SD卡初始化成功，容量：%lu MB (%lu.%lu GB)\r\n",
              total_mb, gb_int, gb_decimal);
      printf("[SD] 块大小: %lu, 总块数: %lu\r\n", 
              card_info.LogBlockSize, card_info.LogBlockNbr);
    }
    else
    {
      printf("[SD] [WARN] 获取SD卡信息失败\r\n");
    }
  }
#endif
  
  return status;
}

/**
  * @brief  检查SD卡状态
  * @retval HAL_StatusTypeDef 返回操作状态
  * @note  如果卡状态为HAL_SD_CARD_DISCONNECTED，返回HAL_TIMEOUT
  * @note  如果卡状态为HAL_SD_CARD_ERROR，返回HAL_ERROR(DEBUG模式下还会输出诊断信息)
  * @note  如果卡状态为HAL_SD_CARD_TRANSFER或HAL_SD_CARD_READY，返回HAL_OK
  * @note  如果卡状态为HAL_SD_CARD_SENDING、HAL_SD_CARD_RECEIVING或HAL_SD_CARD_PROGRAMMING，返回HAL_BUSY
  */
HAL_StatusTypeDef SD_Check(void)
{
  HAL_SD_CardStateTypeDef card_state;
  HAL_StatusTypeDef status = HAL_OK;
  
  card_state = HAL_SD_GetCardState(&hsd1);
  
  switch (card_state)
  {
    case HAL_SD_CARD_TRANSFER:
    case HAL_SD_CARD_READY:
      status = HAL_OK;
      break;
      
    case HAL_SD_CARD_SENDING:
    case HAL_SD_CARD_RECEIVING:
    case HAL_SD_CARD_PROGRAMMING:
      status = HAL_BUSY;
      break;
      
    case HAL_SD_CARD_DISCONNECTED:
      status = HAL_TIMEOUT;
      break;
      
    case HAL_SD_CARD_ERROR:
#ifdef DEBUG
      printf("[SD] SD卡状态异常: %lu\r\n", (uint32_t)card_state);
      SD_ErrorHandler("检查");
#endif
      status = HAL_ERROR;
      break;
      
    default:
      status = HAL_ERROR;
      break;
  }

  return status;
}



/**
  * @brief  等待SD卡就绪
  * @param  Timeout: 超时时间（毫秒）
  * @retval HAL_StatusTypeDef 返回操作状态
  * @note   等待SD卡进入传输状态，支持超时机制
  */
HAL_StatusTypeDef SD_WaitReady(uint32_t Timeout)
{
  uint32_t tickstart_local;
  HAL_StatusTypeDef status = HAL_TIMEOUT;
  
  tickstart_local = HAL_GetTick();
  
  while ((HAL_GetTick() - tickstart_local) < Timeout)
  {
    if (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER)
    {
      status = HAL_OK;
      break;
    }
  }
  
#ifdef DEBUG
  if (status != HAL_OK)
  {
    printf("[SD] [FAIL] 等待SD卡就绪超时 (%lu ms)\r\n", Timeout);
  }
#endif
  
  return status;
}


/**
  * @brief  查询模式多块写入（避免FIFO溢出）
  * @param  pData: 数据缓冲区指针（必须4字节对齐）
  * @param  BlockAdd: 起始块地址
  * @param  NumberOfBlocks: 块数量
  * @param  Timeout: 超时时间（毫秒）
  * @retval HAL_StatusTypeDef 返回操作状态
  * @note   使用查询模式进行多块写入，避免FIFO溢出
  */
HAL_StatusTypeDef SD_WriteBlocks(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout)
{
  HAL_StatusTypeDef status;
  
  /* 参数验证 */
  if (pData == NULL)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 参数错误: pData为NULL\r\n");
#endif
    return HAL_ERROR;
  }
  
  if (NumberOfBlocks == 0U)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 参数错误: NumberOfBlocks为0\r\n");
#endif
    return HAL_ERROR;
  }
  
  /* 等待SD卡就绪 */
  status = SD_WaitReady(Timeout);
  if (status != HAL_OK)
  {
    return status;
  }
  
  /* 关闭中断，避免FIFO溢出 */
  __disable_irq();
  
  /* 多块写入 */
  status = HAL_SD_WriteBlocks(&hsd1, pData, BlockAdd, NumberOfBlocks, Timeout);
  if (status != HAL_OK)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 多块写入失败，状态: %d\r\n", status);
    SD_ErrorHandler("写入");
#endif
  }
  
  /* 重新使能中断 */
  __enable_irq();
  
  return status;
}

/**
  * @brief  查询模式多块读取（避免FIFO溢出）
  * @param  pData: 数据缓冲区指针（必须4字节对齐）
  * @param  BlockAdd: 起始块地址
  * @param  NumberOfBlocks: 块数量
  * @param  Timeout: 超时时间（毫秒）
  * @retval HAL_StatusTypeDef 返回操作状态
  * @note   使用查询模式进行多块读取，避免FIFO溢出
  */
HAL_StatusTypeDef SD_ReadBlocks(uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout)
{
  HAL_StatusTypeDef status;
  
  /* 参数验证 */
  if (pData == NULL)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 参数错误: pData为NULL\r\n");
#endif
    return HAL_ERROR;
  }
  
  if (NumberOfBlocks == 0U)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 参数错误: NumberOfBlocks为0\r\n");
#endif
    return HAL_ERROR;
  }
  
  /* 等待SD卡就绪 */
  status = SD_WaitReady(Timeout);
  if (status != HAL_OK)
  {
    return status;
  }

  /* 关闭中断，避免FIFO溢出 */
  __disable_irq();
  
  /* 多块读取 */
  status = HAL_SD_ReadBlocks(&hsd1, pData, BlockAdd, NumberOfBlocks, Timeout);
  if (status != HAL_OK)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 多块读取失败，状态: %d\r\n", status);
    SD_ErrorHandler("读取");
#endif
  }
  
  /* 重新使能中断 */
  __enable_irq();
  
  return status;
}




#ifdef DEBUG

/* 宏检查：限制测试块数最大值 */
#if SD_TEST_BLOCKS > 256
  #error "SD_TEST_BLOCKS cannot exceed 256 to avoid buffer overflow"
#endif

/* 宏警告：测试块数较少时给出警告 */
#if SD_TEST_BLOCKS < 32
  #warning "SD_TEST_BLOCKS is less than 32. The test data volume is too small and may not accurately reflect SD card performance"
#endif

static uint8_t sd_backup_buf[SD_TEST_BLOCKS * 512];     /* 备份缓冲区 */
static uint8_t sd_read_buf[512];                        /* 读取缓冲区（单块） */

/**
  * @brief  SD卡Polling测试
  * @retval HAL状态
  * @note   检查SD就绪->备份->写入规律数据->读取检验->还原数据->输出速度
  */
HAL_StatusTypeDef SD_MeasureTest(void)
{
    HAL_StatusTypeDef status;
    uint32_t i;
    uint32_t tick_start, tick_end;
    uint32_t write_time_ms, read_time_ms;
    uint32_t backup_read_time_ms;  /* 备份时的读取时间 */
    uint32_t total_bytes = SD_TEST_BLOCKS * SD_BLOCK_SIZE;
    uint32_t verify_errors = 0;
    uint32_t j;
    HAL_SD_CardStateTypeDef card_state;
    
    /* 参数验证 */
    if (SD_TEST_BLOCKS == 0U)
    {
        printf("[SD] [FAIL] 测试块数为0\r\n");
        return HAL_ERROR;
    }
    
    if (SD_TEST_BLOCKS > 256U)
    {
        printf("[SD] [FAIL] 测试块数超过最大限制(256)\r\n");
        return HAL_ERROR;
    }
    
    printf("\r\n========== SD卡Polling测试开始 ==========\r\n\r\n");
    
    /* 1. 检查SD卡就绪 */
    printf("[SD] 检查SD卡就绪状态\r\n");
    card_state = HAL_SD_GetCardState(&hsd1);
    if (card_state != HAL_SD_CARD_TRANSFER)
    {
        printf("[SD] [FAIL] SD卡未就绪，状态: %lu\r\n", (uint32_t)card_state);
        return HAL_ERROR;
    }
    
    /* 2. 备份原始数据 */
    printf("[SD] 备份块%lu-%lu原始数据\r\n",
           (uint32_t)SD_TEST_BLOCK_START, (uint32_t)(SD_TEST_BLOCK_START + SD_TEST_BLOCKS - 1U));
    
    tick_start = HAL_GetTick();
    /* 执行4遍备份 */
    for (j = 0U; j < 4U; j++)
    {
        status = SD_ReadBlocks(sd_backup_buf, SD_TEST_BLOCK_START, SD_TEST_BLOCKS, SD_TIMEOUT_MS);
        if (status != HAL_OK)
        {
            printf("[SD] [FAIL] 备份读取失败: %d\r\n", status);
            return status;
        }
    } 
    
    status = SD_WaitReady(SD_TIMEOUT_MS * 15U);  /* 使用15代替0xF，更符合MISRA-C */
    if (status != HAL_OK) 
    {
        return status;
    }
    
    tick_end = HAL_GetTick();
    backup_read_time_ms = tick_end - tick_start;

    printf("[SD] [PASS] 备份完成(4次)，耗时: %lu ms\r\n", backup_read_time_ms);
    
    /* 3. 准备写入数据：备份值 + 0xA */
    printf("[SD] 准备写入数据（备份值 + 0xA）\r\n");
    for (i = 0U; i < (SD_TEST_BLOCKS * SD_BLOCK_SIZE); i++)
    {
        sd_backup_buf[i] += 0x0AU;  /* 备份值增加0xA作为写入值 */
    }
    
    /* 4. 写入测试数据 - 使用连续多块写入 */
    printf("[SD] 开始写入测试数据（连续多块写入）\r\n");
    tick_start = HAL_GetTick();
    
    /* 使用连续多块写入 */
    /* 执行4次连续多块写入操作 */
    for (j = 0U; j < 4U; j++) 
    {
        status = SD_WriteBlocks(sd_backup_buf, SD_TEST_BLOCK_START, SD_TEST_BLOCKS, SD_TIMEOUT_MS);
        if (status != HAL_OK)
        {
            printf("[SD] [FAIL] 第 %lu 次连续多块写入失败: %d\r\n", (uint32_t)(j + 1U), status);
            goto restore_data;
        }
    }
    
    status = SD_WaitReady(SD_TIMEOUT_MS * 15U);
    if (status != HAL_OK) 
    {
        printf("[SD] [FAIL] 写入完成后SD卡未能恢复就绪\r\n");
        goto restore_data;
    }
    
    tick_end = HAL_GetTick();
    write_time_ms = tick_end - tick_start;
    printf("[SD] [PASS] 写入完成(4次)，耗时: %lu ms\r\n", write_time_ms);
    
    /* 5. 读取并检验数据 */
    printf("[SD] 开始读取并检验数据\r\n");
    
    /* 使用备份时的读取时间，不再重新测量 */
    read_time_ms = backup_read_time_ms;
    
    /* 逐块读取并立即验证 */
    for (i = 0U; i < SD_TEST_BLOCKS; i++)
    {
        status = HAL_SD_ReadBlocks(&hsd1, sd_read_buf, SD_TEST_BLOCK_START + i, 1, SD_TIMEOUT_MS);
        if (status != HAL_OK)
        {
#ifdef DEBUG
          SD_ErrorHandler("读取");
#endif
          goto restore_data;
        }
        
        /* 等待读取完成 */
        status = SD_WaitReady(SD_TIMEOUT_MS);
        if (status != HAL_OK) {
#ifdef DEBUG
            printf("[SD] [FAIL] 块%lu读取超时\r\n", (uint32_t)(SD_TEST_BLOCK_START + i));
            SD_ErrorHandler("读取超时");
#endif
            goto restore_data;
        }
        
        /* 立即验证当前块数据 */
        for (j = 0U; j < SD_BLOCK_SIZE; j++)
        {
            uint8_t expected = sd_backup_buf[(i * SD_BLOCK_SIZE) + j];  /* 期望值就是修改后的备份值 */
            if (sd_read_buf[j] != expected)
            {
                verify_errors++;
                if (verify_errors <= 5U)  /* 只显示前5个错误 */
                {
                    printf("[SD] [FAIL] 块%lu偏移%lu校验错误: 期望0x%02X, 实际0x%02X\r\n",
                           (uint32_t)(SD_TEST_BLOCK_START + i), j, expected, sd_read_buf[j]);
                }
            }
        }
    }
    
    if (verify_errors == 0)
    {
        printf("[SD] [PASS] 数据校验通过 \r\n");
        
        /* 输出读写速度（使用整数运算，保留一位小数精度） */
        /* 由于之前4重执行，等价于传输的字节数x4，因此总数据量需要乘以4 */
        uint32_t total_kb = (total_bytes * 4) / 1024;  /* 总数据量KB */
        
        /* 计算写入速度：使用KB为单位避免溢出，保留一位小数 */
        uint32_t write_speed_kbps = (total_kb * 1000) / write_time_ms;  /* KB/s */
        uint32_t write_speed_kbps_int = write_speed_kbps / 10;  /* 整数部分（除以10用于显示一位小数） */
        uint32_t write_speed_kbps_dec = write_speed_kbps % 10;  /* 小数部分 */
        
        /* 计算读取速度：使用KB为单位避免溢出，保留一位小数 */
        uint32_t read_speed_kbps = (total_kb * 1000) / read_time_ms;  /* KB/s */
        uint32_t read_speed_kbps_int = read_speed_kbps / 10;  /* 整数部分（除以10用于显示一位小数） */
        uint32_t read_speed_kbps_dec = read_speed_kbps % 10;  /* 小数部分 */
        
        /* 计算MB/s速度：MB/s = KB/s / 1024，保留一位小数 */
        uint32_t write_speed_mbps_int = write_speed_kbps / 1024;  /* MB/s整数部分 */
        uint32_t write_speed_mbps_dec = ((write_speed_kbps % 1024) * 10) / 1024;  /* MB/s小数部分 */
        
        uint32_t read_speed_mbps_int = read_speed_kbps / 1024;  /* MB/s整数部分 */
        uint32_t read_speed_mbps_dec = ((read_speed_kbps % 1024) * 10) / 1024;  /* MB/s小数部分 */
        
        printf("[SD] 写入速度: %lu.%lu MB/s (%lu.%lu KB/s)\r\n",
               write_speed_mbps_int, write_speed_mbps_dec, 
               write_speed_kbps_int, write_speed_kbps_dec);
        printf("[SD] 读取速度: %lu.%lu MB/s (%lu.%lu KB/s)\r\n",
               read_speed_mbps_int, read_speed_mbps_dec, 
               read_speed_kbps_int, read_speed_kbps_dec);
        printf("[SD] 总数据量: %lu KB (写入总耗时: %lu ms, 读取总耗时: %lu ms)\r\n",
               total_kb, write_time_ms, read_time_ms);
    }
    else
    {
        printf("[SD] [FAIL] 数据校验失败，错误数: %lu\r\n", verify_errors);
        status = HAL_ERROR;
    }
    
restore_data:
    /* 7. 还原原始数据（无论测试结果如何都要还原） */
    printf("[SD] 还原原始数据（备份值 - 0xA）\r\n");
    
    /* 在还原前，先将备份值减去0xA */
    for (i = 0U; i < (SD_TEST_BLOCKS * SD_BLOCK_SIZE); i++)
    {
        sd_backup_buf[i] -= 0x0AU;
    }
    
    tick_start = HAL_GetTick();
    
    /* 使用连续多块写入还原数据 */
    status = SD_WriteBlocks(sd_backup_buf, SD_TEST_BLOCK_START, SD_TEST_BLOCKS, SD_TIMEOUT_MS);
    if (status != HAL_OK)
    {
#ifdef DEBUG
      SD_ErrorHandler("数据还原");
#endif
      goto end_test;
    }
    
    status = SD_WaitReady(SD_TIMEOUT_MS * 15U);
    if (status != HAL_OK)
    {
      goto end_test;
    }
    
    tick_end = HAL_GetTick();
    if (status == HAL_OK)
    {
        printf("[SD] [PASS] 数据还原完成，耗时: %lu ms\r\n", (uint32_t)(tick_end - tick_start));
    }
    
end_test:
    printf("========== SD卡Polling测试结束 ==========\r\n");
    return status;
}


/**
 * @brief  SD卡错误诊断入口函数
 * @param  operation: 操作类型字符串，如"写入"、"读取"等
 * @retval 无
 * @note   根据 HAL_SD_GetError 返回值，对照完整位表输出详细错误原因
 * @note   符合MISRA-C规范，使用const静态表避免RAM占用
 */
static void SD_ErrorHandler(const char* operation)
{
    uint32_t error_code;
    uint8_t found_error = 0U;
    
    /* 在函数内部获取错误码 */
    error_code = HAL_SD_GetError(&hsd1);
    
    /* 打印基本错误信息 */
    printf("\r\n[SD] === SD卡%s操作[FAIL]诊断开始 ===\r\n", operation);
    printf("[SD] HAL_SD_GetError() = 0x%08lX\r\n", error_code);
    printf("[SD] hsd1.State = %d, hsd1.ErrorCode = 0x%08lX\r\n", hsd1.State, hsd1.ErrorCode);
    
    /* 如果错误码为0，可能是HAL状态错误而非SD错误 */
    if (error_code == 0U)
    {
        printf("[SD] [WARN] SD错误码为0，可能是HAL层返回状态错误\r\n");
        printf("[SD] 建议检查：\r\n");
        printf("[SD] 1. HAL_SD_GetCardState() 返回值\r\n");
        printf("[SD] 2. hsd1.State 和 hsd1.ErrorCode 状态\r\n");
        printf("[SD] 3. 调用栈中的HAL状态返回值\r\n");
        printf("[SD] === SD卡[FAIL]诊断结束 ===\r\n\r\n");
        return;
    }
    
    /* 定义错误码位表 - 使用const静态存储，避免RAM占用 */
    static const struct {
        uint32_t bit;
        const char *name;
        const char *meaning;
        const char *root_cause;
    } err_tbl[] = {
        {0x00000001U, "CCRC_FAIL", "命令响应CRC错误", "信号完整性差/时钟太高"},
        {0x00000002U, "DCRC_FAIL", "数据块CRC错误", "时钟>卡极限/线长阻抗差/未切换高速模式"},
        {0x00000004U, "CTIMEOUT", "命令响应超时", "卡掉线/供电不足/识别阶段时钟>400kHz"},
        {0x00000008U, "DTIMEOUT", "数据超时(DAT0未拉低)", "卡无响应/块地址越界/写保护"},
        {0x00000010U, "TX_UNDERRUN", "发送FIFO下溢", "DMA没跟上/中断打断/时钟太高"},
        {0x00000020U, "RX_OVERRUN", "接收FIFO溢出", "读操作DMA太慢/FIFO阈值设置不当"},
        {0x00000040U, "ADDR_MISALIGNED", "地址未4字节对齐", "缓冲区地址未对齐，需__attribute__((aligned(4)))"},
        {0x00000080U, "BLOCK_LEN", "块长度错误", "块大小≠512字节"},
        {0x00000400U, "WRITE_PROT", "写保护", "卡物理写保护开关打开"},
        {0x00000800U, "LOCK_UNLOCK_FAILED", "锁卡命令失败", "卡已设密码，需CMD42解锁"},
        {0x00001000U, "CARD_IS_LOCKED", "卡处于锁定状态", "卡被锁定，无法操作"},
        {0x00002000U, "CARD_NOT_SUPPORTED", "卡不支持", "电压/功能不匹配"},
        {0x00004000U, "REQUEST_NOT_SUPPORTED", "命令不支持", "发送了非法CMD"},
        {0x00008000U, "INVALID_PARAMETER", "参数无效", "越界/NULL指针"},
        {0x00010000U, "UNSUPPORTED_FEATURE", "功能不支持", "当前传输模式不支持"},
        {0x00020000U, "BUSY", "卡忙", "卡正忙，拒绝新命令"},
        {0x00040000U, "DMA", "DMA错误", "DMA传输中断/TEIF标志"},
        {0x00080000U, "TIMEOUT", "软件超时", "HAL等待事件超时"},
    };
    
    /* 扫描并打印所有置位错误 */
    {
        uint32_t i;
        for (i = 0U; i < (sizeof(err_tbl)/sizeof(err_tbl[0])); i++) 
        {
            if ((error_code & err_tbl[i].bit) != 0U)
            {
                printf("[SD] [ERROR] %s (0x%08lX): %s\r\n", 
                       err_tbl[i].name, err_tbl[i].bit, err_tbl[i].meaning);
                printf("[SD] [原因] %s\r\n", err_tbl[i].root_cause);
                found_error = 1U;
            }
        }
    }
    
    /* 如果有未知错误位，打印警告 */
    {
        uint32_t known_mask = 0U;
        uint32_t i;
        for (i = 0U; i < (sizeof(err_tbl)/sizeof(err_tbl[0])); i++)
        {
            known_mask |= err_tbl[i].bit;
        }
        
        if ((error_code & (~known_mask)) != 0U)
        {
            printf("[SD] [WARN] 检测到未知错误位: 0x%08lX\r\n", 
                   error_code & (~known_mask));
            printf("[SD] [建议] 请检查STM32参考手册SDMMC章节更新错误码表\r\n");
        }
    }
    
    if (found_error == 0U)
    {
        printf("[SD] [WARN] 错误码未匹配到已知错误，可能是组合错误\r\n");
    }
    
    printf("[SD] === SD卡[FAIL]诊断结束 ===\r\n\r\n");
}

#endif /* DEBUG */

/**
 * @brief 获取SD卡信息
 * @param  pCardInfo: 指向SD卡信息结构体的指针
 * @retval HAL_StatusTypeDef 返回操作状态
 * @note 获取SD卡的详细信息，包括容量、类型等
 */
HAL_StatusTypeDef SD_GetCardInfo(SD_CardInfoTypeDef *pCardInfo)
{
  HAL_StatusTypeDef status;
  HAL_SD_CardInfoTypeDef hal_card_info;
  
  /* 参数验证 */
  if (pCardInfo == NULL)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] 参数错误: pCardInfo为NULL\r\n");
#endif
    return HAL_ERROR;
  }
  
  /* 获取HAL层卡信息 */
  status = HAL_SD_GetCardInfo(&hsd1, &hal_card_info);
  if (status != HAL_OK)
  {
#ifdef DEBUG
    printf("[SD] [FAIL] HAL_SD_GetCardInfo失败: %d\r\n", status);
#endif
    return status;
  }
  
  /* 转换为用户结构体格式 */
  pCardInfo->CardType     = hal_card_info.CardType;
  pCardInfo->CardVersion  = hal_card_info.CardVersion;
  pCardInfo->Class        = hal_card_info.Class;
  pCardInfo->RelCardAdd   = hal_card_info.RelCardAdd;
  pCardInfo->BlockNbr     = hal_card_info.BlockNbr;
  pCardInfo->BlockSize    = hal_card_info.BlockSize;
  pCardInfo->LogBlockNbr  = hal_card_info.LogBlockNbr;
  pCardInfo->LogBlockSize = hal_card_info.LogBlockSize;
  
  return HAL_OK;
}

/* USER CODE END 1 */
