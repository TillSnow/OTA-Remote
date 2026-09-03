/**
  ******************************************************************************
  * @file    STM32F4xx_IAP/src/flash_if.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides all the memory related operation functions.
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

/** @addtogroup STM32F4xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t GetSector(uint32_t Address);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
void FLASH_If_Init(void)
{
   HAL_FLASH_Unlock();

  /* Clear pending flags (if any) */
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

}
/**
  * @brief  This function does an erase of all user flash area
  * @param  StartSector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         1: error occurred
  */
uint32_t FLASH_If_Erase(uint32_t StartSector)
{
    uint32_t UserStartSector;
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;

    /* Unlock the Flash to enable the flash control register access *************/

    /* Get the sector where start the user flash area */
    UserStartSector = GetSector(APPLICATION_ADDRESS);

    pEraseInit.TypeErase = TYPEERASE_SECTORS;
    pEraseInit.Sector = UserStartSector;
    pEraseInit.NbSectors = GetSector(USER_FLASH_END_ADDRESS)-UserStartSector+1 ;
    pEraseInit.VoltageRange = VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
    {
        /* Error occurred while page erase */
        return (1);
    }
    return (0);

}

/**
  * @brief  This function erase one sector
  * @param  StartSector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         1: error occurred
  */
uint32_t FLASH_If_Erase_One_Sector(uint32_t StartSector)
{
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;

    pEraseInit.TypeErase = TYPEERASE_SECTORS;
    pEraseInit.Sector = StartSector;
    pEraseInit.NbSectors = 1 ;
    pEraseInit.VoltageRange = VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
    {
        /* Error occurred while page erase */
        return (1);
    }
    return (0);

}

/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  FlashAddress: start address for writing data buffer
  * @param  Data: pointer on data buffer
  * @param  DataLength: length of data buffer (unit is 32-bit word)
  * @retval 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(__IO uint32_t* FlashAddress, uint32_t* Data ,uint32_t DataLength)
{
  uint32_t i = 0;

  for (i = 0; (i < DataLength) && (*FlashAddress <= (USER_FLASH_END_ADDRESS-4)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */
    FLASH_ProgramWord(*FlashAddress, *(uint32_t*)(Data+i));
     /* Check the written value */
      if (*(uint32_t*)*FlashAddress != *(uint32_t*)(Data+i))
      {
        /* Flash content doesn't match SRAM content */
        return(2);
      }
      /* Increment FLASH destination address */
      *FlashAddress += 4;
  }

  return (0);
}

/**
  * @brief  Returns the write protection status of user flash area.
  * @param  None
  * @retval 0: No write protected sectors inside the user flash area
  *         1: Some sectors inside the user flash area are write protected
  */
uint16_t FLASH_If_GetWriteProtectionStatus(void)
{
  uint32_t UserStartSector = FLASH_SECTOR_1;

  /* Get the sector where start the user flash area */
  UserStartSector = GetSector(APPLICATION_ADDRESS);

  /* Check if there are write protected sectors inside the user flash area */
  if ((*(__IO uint16_t *)(OPTCR_BYTE2_ADDRESS) >> (UserStartSector/8)) == (0xFFF >> (UserStartSector/8)))
  { /* No write protected sectors inside the user flash area */
    return 1;
  }
  else
  { /* Some sectors inside the user flash area are write protected */
    return 0;
  }
}

/**
  * @brief  Disables the write protection of user flash area.
  * @param  None
  * @retval 1: Write Protection successfully disabled
  *         2: Error: Flash write unprotection failed
  */
uint32_t FLASH_If_DisableWriteProtection(void)
{
  __IO uint32_t UserStartSector = FLASH_SECTOR_1, UserWrpSectors = OB_WRP_SECTOR_1;

  /* Get the sector where start the user flash area */
  UserStartSector = GetSector(APPLICATION_ADDRESS);

  /* Mark all sectors inside the user flash area as non protected */
  UserWrpSectors = 0xFFF-((1 << (UserStartSector/8))-1);

  /* Unlock the Option Bytes */

  HAL_FLASH_Unlock();
  /* Disable the write protection for all sectors inside the user flash area */
  FLASH_OB_DisableWRP(UserWrpSectors, FLASH_BANK_1);

  /* Start the Option Bytes programming process. */
  if (HAL_FLASH_OB_Launch( ) != HAL_OK)
  {
    /* Error: Flash write unprotection failed */
    return (2);
  }

  /* Write Protection successfully disabled */
  return (1);
}

/**
  * @brief  Gets the sector of a given address
  * @param  Address: Flash address
  * @retval The sector of a given address
  */

static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
	 else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7))*/
  {
    sector = FLASH_SECTOR_7;
  }
//  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
//  {
//    sector = FLASH_SECTOR_7;
//  }
//  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
//  {
//    sector = FLASH_SECTOR_8;
//  }
//  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
//  {
//    sector = FLASH_SECTOR_9;
//  }
//  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
//  {
//    sector = FLASH_SECTOR_10;
//  }
//  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
//  {
//    sector = FLASH_SECTOR_11;
//  }
    return sector;
}


HAL_StatusTypeDef FLASH_OB_DisableWRP(uint32_t WRPSector, uint32_t Banks)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_OB_WRP_SECTOR(WRPSector));
  assert_param(IS_FLASH_BANK(Banks));

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(50000);

  if(status == HAL_OK)
  {
    *(__IO uint16_t*)OPTCR_BYTE2_ADDRESS |= (uint16_t)WRPSector;
  }

  return status;
}


void FLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
  /* Check the parameters */
  assert_param(IS_FLASH_ADDRESS(Address));

  /* If the previous operation is completed, proceed to program the new data */
  CLEAR_BIT(FLASH->CR, FLASH_CR_PSIZE);
  FLASH->CR |= FLASH_PSIZE_WORD;
  FLASH->CR |= FLASH_CR_PG;

  *(__IO uint32_t*)Address = Data;
}



/* ================= CRC32：给固件盖"防伪章" ================= */

uint32_t AB_CRC32_Update(uint32_t crc, const uint8_t *buf, uint32_t len)
{
  uint32_t i;
  uint8_t bit;

  for (i = 0; i < len; i++)
  {
    crc ^= buf[i];                            //传进来CRC，和每一位数据进行异或-------->相同为0，不同为1                         
    for (bit = 0; bit < 8; bit++)
    {
      if (crc & 1u)
        crc = (crc >> 1) ^ 0xEDB88320u;       //再异或这个0xEDB8832u
      else
        crc >>= 1;
    }
  }
  return crc;
}

uint32_t AB_CRC32_Calc(const uint8_t *buf, uint32_t len)
{
  return AB_CRC32_Update(0xFFFFFFFFu, buf, len) ^ 0xFFFFFFFFu;
}


/* ================= 账本：记住"状态 + 指纹 + 长度" ================= */

uint8_t AB_Meta_Write(const AB_Meta_t *meta)
{
  uint32_t addr = META_ADDRESS;
  AB_Meta_t back;

  FLASH_If_Init();                          /* 写Flash前先解锁 */
  if (FLASH_If_Erase_One_Sector(FLASH_SECTOR_2) != 0)
    return 1;                               /* 擦除失败 */

  if (FLASH_If_Write(&addr, (uint32_t *)meta, sizeof(AB_Meta_t) / 4) != 0)
    return 2;                               /* 写入失败 */

  if (AB_Meta_Read(&back) != 0)
    return 3;                               /* 回读失败 */
  if (back.Magic != meta->Magic || back.State != meta->State ||
      back.NewLength != meta->NewLength || back.NewCRC32 != meta->NewCRC32)
    return 3;                               /* 回读内容和写入的不一致 */

  return 0;                                 /* 成功 */
}

uint8_t AB_Meta_Read(AB_Meta_t *meta)
{
  AB_Meta_t *p = (AB_Meta_t *)META_ADDRESS;
  if (p->Magic != AB_MAGIC)
    return 1;                               /* 账本还没写过 / 内容损坏 */
  *meta = *p;
  return 0;
}



/* ================= 提交：把B区固件拷贝到A区并校验 ================= */

static uint32_t AB_CopyBuf[128];   /* 512字节的RAM中转缓冲 */

uint8_t AB_CopyB2A(void)
{
  AB_Meta_t meta;
  uint32_t src_addr = SLOT_B_ADDRESS;
  uint32_t dst_addr = SLOT_A_ADDRESS;
  uint32_t words_left, chunk, i;

  /* 读账本，拿到新固件的长度和指纹 */
  if (AB_Meta_Read(&meta) != 0)
    return 1;
  if (meta.NewLength == 0 || meta.NewLength > SLOT_A_SIZE)
    return 3;

  /* 先写"正在拷贝"：万一中途断电，下次开机就知道A区可能没拷完 */
  meta.State = AB_STATE_COPYING;
  if (AB_Meta_Write(&meta) != 0)
    return 7;

  /* 擦A区（扇区S3，16KB） */
  FLASH_If_Init();
  if (FLASH_If_Erase_One_Sector(FLASH_SECTOR_3) != 0)
    return 4;

  /* 分块拷贝 B -> A（每512字节一块，经RAM中转） */
  words_left = (meta.NewLength + 3) / 4;                     //要拷贝的总量
  while (words_left > 0)
  {
    chunk = (words_left > 128) ? 128 : words_left;           // 当前要拷贝的，最多128
    for (i = 0; i < chunk; i++)
    {
      AB_CopyBuf[i] = *(uint32_t *)(src_addr + i * 4);
    }
    if (FLASH_If_Write(&dst_addr, AB_CopyBuf, chunk) != 0)
      return 5;

    src_addr += chunk * 4;
    words_left -= chunk;
  }

  /* 整体校验：A区的指纹必须和账本一致 */
  if (AB_CRC32_Calc((const uint8_t *)SLOT_A_ADDRESS, meta.NewLength) != meta.NewCRC32)
    return 6;

  /* 拷贝成功，账本改回IDLE（A区现在是正式固件） */
  meta.State = AB_STATE_IDLE;
  if (AB_Meta_Write(&meta) != 0)
    return 7;

  return 0;
}

/* ================= 开机决策：总指挥 ================= */

uint8_t AB_BootReady(void)
{
  AB_Meta_t meta;
  uint32_t msp, pc;

  /* 1. 没有账本：无从判断，返回"不能启动" */
  if (AB_Meta_Read(&meta) != 0)
    return 0;
  if (meta.NewLength == 0 || meta.NewLength > SLOT_A_SIZE)
    return 0;

  /* 2. 按状态处理 */
  if (meta.State == AB_STATE_IDLE)
  {
    /* A区应该等于账本里的指纹 */
    if (AB_CRC32_Calc((const uint8_t *)SLOT_A_ADDRESS, meta.NewLength) != meta.NewCRC32)
    {
      /* A区坏了：B区还留着同一份，重拷一次 */
      if (AB_CopyB2A() != 0)
        return 0;
    }
  }
  else if (meta.State == AB_STATE_COPYING)
  {
    /* 上次拷贝可能没拷完 */
    if (AB_CRC32_Calc((const uint8_t *)SLOT_A_ADDRESS, meta.NewLength) == meta.NewCRC32)
    {
      /* 其实已经拷完了，把账本修正成IDLE */
      meta.State = AB_STATE_IDLE;
      AB_Meta_Write(&meta);
    }
    else
    {
      /* 没拷完：用B区续拷（自愈） */
      if (AB_CopyB2A() != 0)
        return 0;
    }
  }
  else if (meta.State == AB_STATE_READY)
  {
    /* B区有新固件等着提交；A区是没动过的旧固件，直接跑 */
    msp = *(uint32_t *)SLOT_A_ADDRESS;
    pc  = *(uint32_t *)(SLOT_A_ADDRESS + 4);
    if ((msp < 0x20000000u || msp >= 0x20020000u) ||
        (pc  < SLOT_A_ADDRESS || pc >= SLOT_A_ADDRESS + SLOT_A_SIZE))
    {
      /* A区看起来不像程序：用B区验过货的新固件救场 */
      if (AB_CopyB2A() != 0)
        return 0;
    }
  }
  else
  {
    return 0;   /* 未知状态，安全起见不启动 */
  }

  /* 3. 最终安全检查：A区向量表要"像个程序"才能跳 */
  msp = *(uint32_t *)SLOT_A_ADDRESS;
  pc  = *(uint32_t *)(SLOT_A_ADDRESS + 4);
  if ((msp < 0x20000000u || msp >= 0x20020000u) ||
      (pc  < SLOT_A_ADDRESS || pc >= SLOT_A_ADDRESS + SLOT_A_SIZE))
    return 0;

  return 1;
}

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
