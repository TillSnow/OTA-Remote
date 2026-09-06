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
uint8_t boot_in_menu_flag = 0;

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

	HAL_Delay(5000);
	//lcd
	LCD_Init();
	LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
	delay_ms(10);
	LCD_Set_Light(50);


  A_Backup = *(AB_BACKUP_t *)BackUp_addr;
  B_Backup = *(AB_BACKUP_t *)(BackUp_addr + sizeof(AB_BACKUP_t));

  
  //掉电检测区
  /*=============================================*/
  //每次跳转这个数据增加一次，如果==3，代表连续跳转3次，默认APP不能执行
  if(RTC->BKP0R == 3){
    BKP_Count_Clear();
    /*
    APP不能执行
    B->A
    */
    if(AB_Flash_Transmit(B_addr,A_addr,3U,2,&B_Backup,&A_Backup) == 0)
    {
      WDOG_Enable();
      WDOG_Feed();
      uint32_t crc32 = crc32_bitwise((const uint8_t *)A_addr,A_Backup.len);
      if(crc32 == B_Backup.CRC32)
      {
        //A有了来自B的数据
        JUMP_TO_Addr();
      }
      
    }
  }
  /*
  有两种情况
  1、B->A的时候出错                           ------------------------->只需要重新回滚即可
  2、更新版本的时候出错      --------------->因为更新版本代码不会等你                     ------------------------->1、重新更新一次
                                                                                         ------------------------->2、回滚上一个版本                                          
  */
  if(A_Backup.State == Upload_BUSY){
  //接着回滚
  AB_Flash_Transmit(B_addr,A_addr,3U,2,&B_Backup,&A_Backup);
  //不需要跳转，先让B->A,保证后面有可以运行程序，如果是更新版本出错，后面可以按键接着更新
  }
  
  //只有A->B一种错误
  if(B_Backup.State == Upload_BUSY){
    //只需要补充备份，不需要跳转
    AB_Flash_Transmit(A_addr,B_addr,5U,1,&A_Backup,&B_Backup);
  }
  /*=============================================*/

  //按键OTA升级区
  /*=============================================*/
  if(HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == 0){
		// 延时判断是否真的按下
    delay_ms(500);
    if(HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == 0){
      
      LCD_ShowString(72, LCD_H/2, (uint8_t*)"Bootload", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
      LCD_ShowString(32, LCD_H/2+48, (uint8_t*)"OV-Watch V2.4.1", WHITE, BLACK, 24, 0);
			
      boot_in_menu_flag = 1;

      //go in boot menu
      FLASH_If_Init();
      Main_Menu();
    }
  }
  /*=============================================*/


  //跳转区
  /*=================================================*/
  else{
    AB_BACKUP_t *p = (AB_BACKUP_t *)BackUp_addr; 

    if(p->flag == APP_FLAG){
      //表示已经有了APP
      //开启看门狗
      BKP_Count_Add();
      WDOG_Enable();
      WDOG_Feed();
      JUMP_TO_Addr();
    }
    //如果没有
    else{
      //表示后面没有APP，此时又没有APP，又没有进入bootloader
      //显示字符串关机了
      LCD_ShowString(74, LCD_H/2, (uint8_t*)"No App!", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
	    LCD_ShowString(32, LCD_H/2+48, (uint8_t*)"Please Download", WHITE, BLACK, 24, 0);
	    HAL_Delay(1000);
    }
  }
  /*=================================================*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1){
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
