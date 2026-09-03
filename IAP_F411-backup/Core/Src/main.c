/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "menu.h"
#include "delay.h"
#include "lcd.h"
#include "lcd_init.h"
#include "power.h"
#include "key.h"
#include "KT6328.h"
#include <string.h>
#include <stdio.h>
#include "daydream_OTA.h"
#include "WDOG.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */





/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	SCB->VTOR = FLASH_BASE;
  extern FileName;
  extern Number;
  extern flashdestination_N;
  extern flashdestination_L;
  extern tab_1024[1024];
  extern AB_CRC Temp1;
  AB_CRC Temp2;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

	//sys delay
	delay_init();

  //key
	Key_Port_Init();

	//BLE
	KT6328_GPIO_Init();
	KT6328_Enable();
  
  //WDOG
  WDOG_Port_Init();
  WDOG_Disnable();
	
	//power GPIO使能和电池供电使能
	Power_Init();

	//PWM Start
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

	//lcd
	LCD_Init();
	LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
	delay_ms(10);
	LCD_Set_Light(50);

  if(!RTC->BKP0R)
  {
    //在APP必须要写相应的代码，让RTC->BKPOR的值clear
    //在bootloader刚上电的时候，理论上BKPOR的值应该是0，如果不是0，就是APP没有喂狗，掉了，说明A用不了
    //这里要开始将A的值传给B
    //这里不仅用了A还用了B，所以需要修改他们的状态值
    Temp1 = New_CRC;                                                                                                    //可能有问题
    Temp2 = Last_CRC;
    Temp1.State = Upload_BUSY;
    Temp1.State = Upload_BUSY;
    HAL_FLASH_Unlock();
    FLASH_If_Write(&flashdestination_N, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
    FLASH_If_Write(&flashdestination_L, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
    HAL_FLASH_Lock();
    while(1)
    {
      flash_transmit(B_addr,A_addr,3U,1);
      if(Last_CRC.CRC32 == New_CRC.CRC32) break; 
    }
    Temp1.State = Upload_IDLE;                                                                                          //可能有问题
    Temp1.State = Upload_IDLE;                                                                                          //可能有问题
    HAL_FLASH_Unlock();
    FLASH_If_Write(&flashdestination_N, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
    FLASH_If_Write(&flashdestination_L, (uint32_t *)&New_CRC, sizeof(New_CRC)) / 4  == 0;
    
    
  }


  //上电的第一时间应该检测state是不是处在BUSY，如果在BUSY说明升级的时候掉电了
  if(New_CRC.State == Upload_BUSY)
  {
    //说明在传输的时候掉电了应该去接着传输
    New_CRC.len = Ymodem_Receive(&tab_1024[0]);
    //这里结束了代表传输完成了
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
  if(Last_CRC.State = Upload_BUSY)
  {
      //B传A也出了问题
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
  }

  //开机启动时如果按下KEY1, 进入boot中IAP升级模式
  if(HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == 0)
  {
		// 延时判断是否真的按下
    delay_ms(500);
    if(HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == 0)
    {
      
      LCD_ShowString(72, LCD_H/2, (uint8_t*)"Bootload", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
      LCD_ShowString(32, LCD_H/2+48, (uint8_t*)"OV-Watch V2.4.1", WHITE, BLACK, 24, 0);

      //go in boot menu
      FLASH_If_Init();
      Main_Menu();
    }
  }
  
  /*=================================================*/
  /*
  逻辑改了，如果没有按下KEY1
  1.去校验层找Magic，如果==0x12345678，说明有APP
  2.找AB_flag
  3.跳
  */
  else
  {
    //找Magic------怎么找
    AB_CRC *p = (AB_CRC *)CRC_addr; 

    if(p->Magic == 0x12345678)
    {
      //现在缺什么，正常情况A运行，需要让A->B进行备份，应该放在什么位置，放在这里每一次上电都进行备份一次？去bootloader，检测Last_CRC的魔数如果。。。。。
      //表示已经有了APP
      //开启看门狗
      BKP_Count_Clear();
      BKP_Count_Add();
      WDOG_Enable();
      WDOG_Feed();
      JUMP_TO_Addr();
    }
    //如果没有
    else
    {
      //表示后面没有APP，此时又没有APP，又没有进入bootloader
      //显示字符串关机了
      LCD_ShowString(74, LCD_H/2, (uint8_t*)"No App!", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
      LCD_ShowString(32, LCD_H/2+48, (uint8_t*)"Please Download", WHITE, BLACK, 24, 0);
	    HAL_Delay(1000);
    }

  }
  /*=================================================*/



  // //如果没按下KEY1, 且有APP程序, 则运行APP, 没有APP则
  // else
  // {
  //   uint32_t data1, data2;
  //   char *str_flag;
  //   uint32_t address = 0x08008000; // Flash 中数据的起始地址

  //   // 读取地址为 0x08008000 的数据
  //   data1 = *(uint32_t *)address;

  //   // 读取地址为 0x08008004 的数据
  //   data2 = *(uint32_t *)(address + sizeof(uint32_t));

  //   // 将数据转换为字符数组
  //   char str1[5], str2[5];
  //   memcpy(str1, &data1, sizeof(data1));
  //   memcpy(str2, &data2, sizeof(data2));
  //   str1[4] = '\0';
  //   str2[4] = '\0';

  //   // 拼接两个字符数组
  //   char combined_str[9];
  //   strcpy(combined_str, str1);
  //   strcat(combined_str, str2);

  //   // 检查是否与 "APP FLAG" 相同
  //   if (strcmp(combined_str, "APP FLAG") == 0)
  //   {
  //       // 如果相同则跳转
  //       printf("APP FLAG OK, jump to app\r\n");
  //       //user code here
  //       SysTick->CTRL = 0X00;//禁止SysTick
  //       SysTick->LOAD = 0;
  //       SysTick->VAL = 0;
  //       __disable_irq();

  //       //set JumpAddress
  //       JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
  //       /* Jump to user application */
  //       Jump_To_Application = (pFunction) JumpAddress;
  //       /* Initialize user application's Stack Pointer */
  //       __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
  //       Jump_To_Application();
  //   }
	// 	// no legal APP 
	// 	else
	// 	{
	// 		LCD_ShowString(74, LCD_H/2, (uint8_t*)"No App!", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
  //     LCD_ShowString(32, LCD_H/2+48, (uint8_t*)"Please Download", WHITE, BLACK, 24, 0);
	// 		HAL_Delay(1000);
	// 	}
  // }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		printf("run in boot while(1)\r\n");
    printf("there is no legal APP\r\n");
		HAL_Delay(500);
		Power_DisEnable();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
