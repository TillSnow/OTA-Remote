/**
  ******************************************************************************
  * @file    STM32F4xx_IAP/inc/flash_if.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides all the headers of the flash_if functions.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_IF_H
#define __FLASH_IF_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "main.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbyte */
//#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbyte */
//#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbyte */
//#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbyte */

/* End of the Flash address */
#define USER_FLASH_END_ADDRESS        0x0807FFFF
/* Define the user application size */
#define USER_FLASH_SIZE   (USER_FLASH_END_ADDRESS - APPLICATION_ADDRESS + 1)

/* Define the address from where user application will be loaded.
   Note: the 1st sector 0x08000000-0x08003FFF is reserved for the IAP code */
#define APPLICATION_ADDRESS   (uint32_t)0x0800C000

/* ================= A/B 双区定义（学习版） ================= */

/* A区：运行区，就是原来的APP地址（扇区S3，16KB） */
#define SLOT_A_ADDRESS   (uint32_t)0x0800C000
#define SLOT_A_SIZE      (uint32_t)0x00004000   /* 16KB */

/* B区：暂存区（扇区S4，64KB），新固件先收在这里 */
#define SLOT_B_ADDRESS   (uint32_t)0x08010000
#define SLOT_B_SIZE      (uint32_t)0x00010000   /* 64KB */

/* 状态区：原来的 "APP FLAG" 扇区（S2，16KB），改成"账本" */
#define META_ADDRESS     (uint32_t)0x08008000

/* Ledger: single copy (learning version) */
typedef struct {
  uint32_t Magic;      /* 魔数，固定 0xAB1234AB，用来判断状态区有没有被写过 */
  uint32_t State;      /* 0=空闲(A区有效)  1=新固件已就绪待提交  2=拷贝中/需要续拷 */
  uint32_t NewLength;  /* B区里新固件的字节数 */
  uint32_t NewCRC32;   /* B区里新固件的CRC32 */
} AB_Meta_t;

/* =========================================================== */

/* 状态值 */
#define AB_STATE_IDLE      0u   /* 空闲：A区是上次提交的固件 */
#define AB_STATE_READY     1u   /* B区已收好新固件，等待提交 */
#define AB_STATE_COPYING   2u   /* 正在把B拷到A，掉电后需要续拷 */

/* 魔数：判断状态区有没有被正常写过 */
#define AB_MAGIC           0xAB1234ABu

/* 函数声明：CRC32（整包"指纹"） */
uint32_t AB_CRC32_Update(uint32_t crc, const uint8_t *buf, uint32_t len);
uint32_t AB_CRC32_Calc(const uint8_t *buf, uint32_t len);

/* 函数声明 */
uint8_t  AB_Meta_Write(const AB_Meta_t *meta);
uint8_t  AB_Meta_Read(AB_Meta_t *meta);

/* 函数声明：提交（拷贝B→A并校验） */
uint8_t AB_CopyB2A(void);

/* 函数声明：开机决策（校验A区，坏了用B区自愈；返回1=可以跳A） */
uint8_t AB_BootReady(void);

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void FLASH_If_Init(void);
uint32_t FLASH_If_Erase(uint32_t StartSector);
uint32_t FLASH_If_Erase_One_Sector(uint32_t StartSector);
uint32_t FLASH_If_Write(__IO uint32_t* FlashAddress, uint32_t* Data, uint32_t DataLength);
uint16_t FLASH_If_GetWriteProtectionStatus(void);
uint32_t FLASH_If_DisableWriteProtection(void);

//
void FLASH_ProgramWord(uint32_t Address, uint32_t Data);
HAL_StatusTypeDef FLASH_OB_DisableWRP(uint32_t WRPSector, uint32_t Banks);

#endif  /* __FLASH_IF_H */

/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE******/
