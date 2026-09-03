#include "daydream_OTA.h"


static uint32_t JumpAddress;
AB_CRC Last_CRC = {0,0,0,Upload_IDLE};
AB_CRC New_CRC = {0,0,0,Upload_IDLE};

void JUMP_TO_Addr()
{
    //先关systick中断
    SysTick->CTRL = 0x00;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __disable_irq();
    JumpAddress = A_addr;                            //这是分配的A区的地址
    Reset_Addr = *(uint32_t *)(JumpAddress + 4);            //这是Reset程序所在的地址，但是要的是地址下面存储的东西
    Jump_To_App = Reset_Addr;
    __set_MSP(*(uint32_t *)JumpAddress);
    Jump_To_App();
  
}
  

/*
第一次运行直接给A
第二次运行复制给B再让新数据给A
如果出问题B->A
这个函数解决的是AB分区之间的数据传递
*/
void flash_transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_CRC *C)
{   
    //每次只能传输4个字节
    uint32_t buf[Buf_Num] = {0};
    uint8_t give_Data;
    uint8_t Remain;
    uint32_t temp;

    //首先需要打开flash
    FLASH_If_Init();

    //首先写入是有限制的，每次只能写4个字节，但是读没有限制，首先假设有x个字节，缓冲区有512个字节
    give_Data = C->len / 512; 
    //但是如果有1025个字节，就会导致漏
    if(Remain = C->len % 512 > 0)give_Data += 1;
    while(give_Data--)
    {
        //将地址里面的数据存储进数组
        uint32_t SectorError;
        uint16_t i,j = 0;
        for(i = 0;i < Buf_Num;i++)
        {
            
            buf[i] = *(uint32_t *)(Taddr + j);
            j += 4;
        }
        flash_Rx.TypeErase = FLASH_TYPEERASE_SECTORS;
        flash_Rx.Sector = erase;
        flash_Rx.NbSectors = len;
        flash_Rx.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        //存进数组之后应该擦除要传输的
        HAL_FLASHEx_Erase(&flash_Rx,&SectorError);
        //擦除了扇区之后应该是往里面写了
        //第一种，使用HAL库，每一次写入32位数据，共传输128次
        // for(i = 0;i < Buf_Num;i++)
        // {
        //     HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,Raddr,buf[i]);
        // }
        //第二种，怎么读的怎么写进去
        for(i = 0;i < Buf_Num;i++)
        {
            j = 0;
            *(__IO uint32_t *)(Raddr + j) = buf[i];
        }
        
    }
    //写完关掉
    HAL_FLASH_Lock();
}

//CRC32校验
//需要了解CRC校验
//异或的数据可以随意选择，但是一般选择0x04c11db7
// void CRC32()
// {

// }



//了解到的
uint32_t crc32_bitwise(const uint8_t *data,size_t length)
{
    uint32_t crc = 0xffffffff;
    for(size_t i = 0; i < length;i++)
    {
        crc ^= data[i];
        for(int j = 0;j < 8;j++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >> 1;
        }
    }
    return crc ^ 0xffffffff;
}


void BKP_Count_Add(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  RTC->BKP0R = RTC->BKP0R + 1;
  HAL_PWR_DisableBkUpAccess();
}
void BKP_Count_Clear(void)   // APP 里也要用，同样的函数
{
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  RTC->BKP0R = 0;
  HAL_PWR_DisableBkUpAccess();
}


