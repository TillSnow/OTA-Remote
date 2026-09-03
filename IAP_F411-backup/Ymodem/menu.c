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



/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

pFunction Jump_To_Application;
uint32_t JumpAddress;
__IO uint32_t FlashProtection = 0;
uint8_t tab_1024[1024] = {0};
uint8_t FileName[FILE_NAME_LENGTH];
uint32_t flashdestination_N = ADDR_FLASH_SECTOR_2;                    //New_CRC存放的地址
uint32_t flashdestination_L = ADDR_FLASH_SECTOR_2 + sizeof(New_CRC);  //Last_CRC存放的地址
AB_CRC Temp1;

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
  
  // // sector 2
  FLASH_If_Erase_One_Sector(2U);

  /* Waiting for file sent */
  SerialPutString("Waiting for the file to be sent ... (press 'a' to abort)\n\r");

  New_CRC.State = Upload_BUSY;                                    //应该再找一个地方写进去
  HAL_FLASH_Unlock();
  FLASH_If_Write(&flashdestination_N, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
  HAL_FLASH_Lock();
  New_CRC.len = Ymodem_Receive(&tab_1024[0]);
  New_CRC.State = Upload_IDLE;
  New_CRC.CRC32 = crc32_bitwise((const uint8_t *)APPLICATION_ADDRESS,New_CRC.len);
    New_CRC.Magic = 0x12345678;
  //成功时返回文件的大小到New_CRC.len
  if (New_CRC.len > 0)
  {
    SerialPutString("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    SerialPutString(FileName);
    Int2Str(Number, New_CRC.len);
    SerialPutString("\n\r New_CRC.len: ");
    SerialPutString(Number);
    SerialPutString(" Bytes\r\n");
    SerialPutString("-------------------\n");
    // user operation
    // set the flag in flash in 0x08008000
    // const char *str_flag = "APP FLAG";
    // uint32_t * APP_FLAG = (uint32_t *)str_flag;
    
    /* Write received data in Flash */
    if (FLASH_If_Write(&flashdestination_N, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0)
   {
      SerialPutString("\n\n\r New_CRC Set Successfully!\n\r--------------------------------\r\n");
    }
    
    else /* An error occurred while writing to Flash memory */
    {
      /* End session */
      SerialPutString("\n\n\r APP Flag Set Error!\n\r--------------------------------\r\n");
    }

  }




  //如果有问题
  else if (New_CRC.len == -1)
  {
    SerialPutString("\n\n\rThe image New_CRC.len is higher than the allowed space memory!\n\r");
  }
  else if (New_CRC.len == -2)
  {
    SerialPutString("\n\n\rVerification failed!\n\r");
  }
  else if (New_CRC.len == -3)
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
    status = Ymodem_Transmit((uint8_t*)APPLICATION_ADDRESS, (const uint8_t*)"UploadedFlashImage.bin", New_CRC.len);

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

    if (key == 0x31)
    {
      
      /* Download user application in the Flash */
      /*这里是重点，在这里下载程序
      我要实施AB分区，加上回滚机制
      其实也挺简单的
      AB分区，在APP层创建两个用来存放数据的地方，记住他们的起始地址，如果不需要回滚机制，就A一直运行，B用作备份，每次更新就进B去
      我想到了，可能可以更简单一点，
      A用作运行区，B用作备份区，每一次更新直接到A，和之前一样，只是多了个备份区B，
      */

      /*在前面应该有
      这里应该是检测是否需要了，需要再烧
      怎么算需要
      其实除了第二次，以后不管怎么样都要烧，都是直接烧
      只有第一次和第二次是例外
      */

      //这里是对A烧录程序
      SerialDownload();

      /*还应该想到的问题
      1、如果去备份没有成功，此时应该还在Upload_BUSY
      2、如果A的数据出现了问题，B应该传给A
      */
      //烧录B
      Last_CRC.State = Upload_BUSY;
      Temp1 = New_CRC; 
      //这个时候还应该开flash
      HAL_FLASH_Unlock();
      //是不是还应该擦除，擦除了New就每了，还得先保存一下New
      FLASH_If_Erase_One_Sector(2U);
      //写进去
      FLASH_If_Write(&flashdestination_N, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
      FLASH_If_Write(&flashdestination_L, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
      //这里应该上锁了
      HAL_FLASH_Lock();
      
      while(1)
      {
        //此时B没有被烧录，上面烧录了A，这个时候应该备份一次
        flash_transmit(A_addr,B_addr,4U,1);
        //是不是应该算一下CRC
        if(crc32_bitwise(B_addr,New_CRC.len) == New_CRC.CRC32)break;;
      }
      HAL_FLASH_Unlock();
      Last_CRC.State = Upload_IDLE;
      FLASH_If_Erase_One_Sector(2U);
      //写进去
      FLASH_If_Write(&flashdestination_N, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
      FLASH_If_Write(&flashdestination_L, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;    
      HAL_FLASH_Lock();

      /*还应该检测是否需要备份
      应该是什么样子的？
      第一次上电，A被烧录------->备份B                         ------------->第一次反而不行，需要先烧录A，再备份B
      第二次烧录，A被烧录------->B留着上一次的留着回滚          ------------>也能先检测是否需要，A再去烧录
      第三次烧录，A被烧录------->B烧录第二次A的                 ------------>这里应该先传给B，再去烧录
      */

      /*
      不对啊，好像思路有问题
      第一次上电，此时没有APP程序   -------------->烧录A-------------------->烧录B
      第二次上电，此时有了APP程序   -------------->烧录A-------------------->烧录B
      第三次上电，此时有了APP程序   -------------->烧录A-------------------->烧录B
      第四次上电，此时有了APP程序   -------------->烧录A-------------------->烧录B
      第五次上电，此时有了APP程序   -------------->烧录A-------------------->烧录B
      第六次上电，此时有了APP程序   -------------->烧录A-------------------->烧录B
      如果在烧录新程序的时候，A出了问题，就没到烧录B的时候，直接回滚不就行了么
      */

    }
    else if (key == 0x32)
    {
      /* Upload user application from the Flash */
      // SerialUpload();
      SerialPutString("This function is disabled! Please Use 1\r");
    }
    else if (key == 0x33) /* execute the new program */
    {
      AB_CRC *p = (AB_CRC *)CRC_addr; 

      if(p->Magic == 0x12345678)
      {
        //表示已经有了APP
        //如果是0，表示运行A
        JUMP_TO_A();
      }
      // //user code here
      // SysTick->CTRL = 0X00;//禁止SysTick
      // SysTick->LOAD = 0;
      // SysTick->VAL = 0;
      // __disable_irq();

      // //set JumpAddress
      // JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      // /* Jump to user application */
      // Jump_To_Application = (pFunction) JumpAddress;
      // /* Initialize user application's Stack Pointer */
      // __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      // Jump_To_Application();
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
