#include "daydream_OTA.h"


FLASH_EraseInitTypeDef flash_Rx;
AB_BACKUP_t A_Backup = {0,0,0,Upload_IDLE};
AB_BACKUP_t B_Backup = {0,0,0,Upload_IDLE};
static uint32_t JumpAddress;


void BKP_Count_Add(void){
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  RTC->BKP0R = RTC->BKP0R + 1;
  HAL_PWR_DisableBkUpAccess();
}
void BKP_Count_Clear(void){   // APP 里也要用，同样的函数{
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  RTC->BKP0R = 0;
  HAL_PWR_DisableBkUpAccess();
}

/**
 * @brief   跳转A区
*/
void JUMP_TO_Addr(void){
    JumpAddress = A_addr;                            //这是分配的A区的地址
    uint32_t Reset_Addr = *(uint32_t *)(JumpAddress + 4);            //这是Reset程序所在的地址，但是要的是地址下面存储的东西
    
    //先关systick中断
    SysTick->CTRL = 0x00;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __disable_irq();
    pFunction Jump_To_App = (pFunction)Reset_Addr;
    __set_MSP(*(uint32_t *)JumpAddress);
    Jump_To_App();
}


/**
 * @brief   算出CRC32值
 * @param   data:数据
 * @param   length:长度
*/
uint32_t crc32_bitwise(const uint8_t *data,size_t length){
    uint32_t crc = 0xffffffff;
    for(size_t i = 0; i < length;i++){
        crc ^= data[i];
        for(int j = 0;j < 8;j++){
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xffffffff;
}


/**
 * @brief 赋值AB备份区
*/
void AB_Backup_Flash_Write(void){
    uint32_t addr = N_addr;
    HAL_FLASH_Unlock();
    FLASH_If_Erase_One_Sector(2U);
    FLASH_If_Write(&addr,(uint32_t *)&A_Backup,sizeof(AB_BACKUP_t) / 4);
    FLASH_If_Write(&addr,(uint32_t *)&B_Backup,sizeof(AB_BACKUP_t) / 4);
    HAL_FLASH_Lock();
}

/**
 * @brief   传数据
 * @param   Taddr:发送方
 * @param   Raddr:接收方
 * @param   erase:擦除的扇区
 * @param   len:连着擦多少扇区
 * @param   T:传过去的备份区
*/
void flash_transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_BACKUP_t *T){   
    //每次只能传输4个字节
    uint32_t buf[Buf_Num] = {0};
    uint8_t give_Data;
    uint8_t Remain;
    uint32_t temp = Buf_Num;
    uint32_t SectorError;
    //首先写入是有限制的，每次只能写4个字节，但是读没有限制，首先假设有x个字节，缓冲区有512个字节
    give_Data = T->len / 512; 
    //但是如果有1025个字节，就会导致漏
    if((Remain = T->len % 512) > 0)give_Data += 1;
    
    //首先需要打开flash
    FLASH_If_Init();
    flash_Rx.TypeErase = FLASH_TYPEERASE_SECTORS;
    flash_Rx.Sector = erase;
    flash_Rx.NbSectors = len;
    flash_Rx.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    //存进数组之后应该擦除要传输的
    HAL_FLASHEx_Erase(&flash_Rx,&SectorError);
    while(give_Data--){
        //将地址里面的数据存储进数组
        uint16_t i,j = 0;
        if(give_Data == 1)temp = Remain / 4;
        for(i = 0;i < temp;i++){
            buf[i] = *(uint32_t *)(Taddr + j);
            j += 4;
        }
        j = 0;
        
        for(i = 0;i < temp;i++){
            /* Check the parameters */
            assert_param(IS_FLASH_ADDRESS(Raddr));
            /* If the previous operation is completed, proceed to program the new data */
            CLEAR_BIT(FLASH->CR, FLASH_CR_PSIZE);
            FLASH->CR |= FLASH_PSIZE_WORD;                      //PSIZE--->第8，第9位，设置成了10  传字    ，00：位   01：半节     10：字    11：双字
            FLASH->CR |= FLASH_CR_PG;                           //PG位---->第一位     0：进制编程   1：使能编程

            *(__IO uint32_t *)(Raddr + j) = buf[i];
            j += 4;
        }
        Taddr += temp * 4;
        Raddr += temp * 4;
    }
    //写完关掉
    HAL_FLASH_Lock();
}

/**
 * @brief   AB区互相传数据
 * @param   Taddr:发送方
 * @param   Raddr:接收方
 * @param   erase:擦除的扇区
 * @param   len:连着擦多少扇区
 * @param   T:传过去的备份区
 * @param   R:接收的的备份区
*/
void AB_Flash_Transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_BACKUP_t *T,AB_BACKUP_t *R){
    //首先是清除计数
    BKP_Count_Clear();
    R->State = Upload_BUSY;
    AB_Backup_Flash_Write();
    while(1){
        flash_transmit(Taddr,Raddr,erase,len,T);
        R->len = T->len;
        uint32_t temp = crc32_bitwise((const uint8_t *)Raddr,R->len);
        //算出来了接收方的CRC值，需要让这个值和接收放的进行对比，但是怎样知道传递过来的CRC在哪里
        //可以使用笨方法
        if(T->CRC32 == temp) {
            R->CRC32 = temp;
            break;
        }
    }
    R->State = Upload_IDLE;
    AB_Backup_Flash_Write();
}
