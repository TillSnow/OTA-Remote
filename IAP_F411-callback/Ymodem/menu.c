/**
  ******************************************************************************
  * @file    STM32F4xx_IAP/src/menu.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    10-October-2011
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file,
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded
  *             - disabling the write protection of the Flash sectors where the
  *               user loads his binary file.
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
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"
#include "daydream_OTA.h"
#include "WDOG.h"



/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint32_t JumpAddress;
__IO uint32_t FlashProtection = 0;
uint8_t tab_1024[1024] = {0};
uint8_t FileName[FILE_NAME_LENGTH];
/* Private function prototypes -----------------------------------------------*/
void SerialDownload(void);
void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void SerialDownload(void)
{
  uint8_t Number[10] = "          ";
  // int32_t size = 0;

  /*======================这里是擦除扇区2，但是应该根据flag擦除=============================*/
  // // user operation
  // // clear the flag in flash in 0x08008000

  /* Waiting for file sent */
  SerialPutString("Waiting for the file to be sent ... (press 'a' to abort)\n\r");

  A_Backup.State = Upload_BUSY;                                    //应该再找一个地方写进去
  AB_Backup_Flash_Write();
  FLASH_If_Init();
  A_Backup.len = Ymodem_Receive(&tab_1024[0]);
  
  //成功时返回文件的大小到New_CRC.len
  if (A_Backup.len > 0){
    SerialPutString("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    SerialPutString(FileName);
    Int2Str(Number, A_Backup.len);
    SerialPutString("\n\r A_Backup.len: ");
    SerialPutString(Number);
    SerialPutString(" Bytes\r\n");
    SerialPutString("-------------------\n");
    // user operation
    A_Backup.State = Upload_IDLE;
    A_Backup.CRC32 = crc32_bitwise((const uint8_t *)A_addr,A_Backup.len);
    A_Backup.flag = APP_FLAG;
    AB_Backup_Flash_Write();
  }
  //如果有问题
  else if (A_Backup.len == -1)
  {
    SerialPutString("\n\n\rThe image A_Backup.len is higher than the allowed space memory!\n\r");
  }
  else if (A_Backup.len == -2)
  {
    SerialPutString("\n\n\rVerification failed!\n\r");
  }
  else if (A_Backup.len == -3)
  {
    SerialPutString("\r\n\nAborted by user.\n\r");
  }
  else
  {
    SerialPutString("\n\rFailed to receive the file!\n\r");
  }
}

/**
  * @brief  Upload a file via serial port.
  * @param  None
  * @retval None
  */
void SerialUpload(void)
{
  uint8_t status = 0 ;

  SerialPutString("\n\n\rSelect Receive File\n\r");

  if (GetKey() == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t*)APPLICATION_ADDRESS, (const uint8_t*)"UploadedFlashImage.bin", A_Backup.len);

    if (status != 0)
    {
      SerialPutString("\n\rError Occurred while Transmitting File\n\r");
    }
    else
    {
      SerialPutString("\n\rFile uploaded successfully \n\r");
    }
  }
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
  uint8_t key = 0;
  int8_t ret = 0;
  SerialPutString("\r\n======================================================================");
  SerialPutString("\r\n=              (C) COPYRIGHT 2011 STMicroelectronics                 =");
  SerialPutString("\r\n=                                                                    =");
  SerialPutString("\r\n=  STM32F4xx In-Application Programming Application  (Version 1.0.0) =");
  SerialPutString("\r\n=                                                                    =");
  SerialPutString("\r\n=                                   By MCD Application Team          =");
  SerialPutString("\r\n======================================================================");
  SerialPutString("\r\n\r\n");

  /* Test if any sector of Flash memory where user application will be loaded is write protected */
  if (FLASH_If_GetWriteProtectionStatus() == 0)
  {
    FlashProtection = 1;
  }
  else
  {
    FlashProtection = 0;
  }

  while (1)
  {
    SerialPutString("\r\n================== Main Menu ============================\r\n\n");
    SerialPutString("  Download Image To the STM32F4xx Internal Flash ------- 1\r\n\n");      //下载新程序
    SerialPutString("  Upload Image From the STM32F4xx Internal Flash ------- 2\r\n\n");      //上传现有程序
    SerialPutString("  Execute The New Program ------------------------------ 3\r\n\n");      //退出

    if(FlashProtection != 0)
    {
      SerialPutString("  Disable the write protection ------------------------- 4\r\n\n");  //重新解除flash保护
    }

    SerialPutString("==========================================================\r\n\n");

    /* Receive key */
    key = GetKey();

    if (key == 0x31){
      //第二次烧录：A更新，B留备份
      if(A_Backup.CRC32 == B_Backup.CRC32)SerialPutString("Don't need to update\r");
      //第二次之后：B留A备份，A更新
      else if(B_Backup.flag == APP_FLAG){
        if(A_Backup.CRC32 != B_Backup.CRC32){
          B_Backup.State = Upload_BUSY;
          AB_Backup_Flash_Write();
          ret = AB_Flash_Transmit(A_addr,B_addr,5U,1,&A_Backup,&B_Backup);
          if(ret == 0){
            B_Backup.State = Upload_IDLE;
            AB_Backup_Flash_Write();
            SerialPutString("Backup sucessfull\r");
          }
          else if(ret == -1){
              SerialPutString("len error\r");
          }
          else if(ret == -2){
              SerialPutString("crc error\r");
          }
        }
      }
      //这里是对A烧录程序
      SerialDownload();
      //第一次：A更新，A->B(只需要执行一次)
      if(B_Backup.flag != APP_FLAG){
        B_Backup.State = Upload_BUSY;
        AB_Backup_Flash_Write();
        ret = AB_Flash_Transmit(A_addr,B_addr,5U,1,&A_Backup,&B_Backup);
        if(ret == 0){
           B_Backup.State = Upload_IDLE;
           B_Backup.flag = APP_FLAG;
           AB_Backup_Flash_Write();
           SerialPutString("Backup sucessfull\r");
        }
       else if(ret == -1){
           SerialPutString("len error\r");
         }
       else if(ret == -2){
           SerialPutString("crc error\r");
         }
       }
      }
    else if (key == 0x32)
    {
      /* Upload user application from the Flash */
      // SerialUpload();
      SerialPutString("This function is disabled! Please Use 1\r");
    }
    else if (key == 0x33) /* execute the new program */
    {
      if(A_Backup.flag == APP_FLAG){
        WDOG_Enable();
        WDOG_Feed();
        //表示已经有了APP
        JUMP_TO_Addr();
      }

    }
    else if ((key == 0x34) && (FlashProtection == 1))
    {
      /* Disable the write protection */
      switch (FLASH_If_DisableWriteProtection())
      {
        case 1:
        {
          SerialPutString("Write Protection disabled...\r\n");
          FlashProtection = 0;
          break;
        }
        case 2:
        {
          SerialPutString("Error: Flash write unprotection failed...\r\n");
          break;
        }
        default:
        {
        }
      }
    }
    else
    {
      if (FlashProtection == 0)
      {
        SerialPutString("Invalid Number ! ==> The number should be either 1, 2 or 3\r");
      }
      else
      {
        SerialPutString("Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
      }
    }
  }
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2011 STMicroelectronics *****END OF FILE******/
