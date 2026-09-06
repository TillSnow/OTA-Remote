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
        crc ^= *(data + i);
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
    uint32_t addr = BackUp_addr;
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
int8_t flash_transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_BACKUP_t *T){   
    //每次只能传输4个字节
    uint32_t buf[Buf_Num] = {0};
    uint8_t give_Data = 0;
    uint16_t Remain;
    uint8_t Remain_buf;
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

    if(HAL_FLASHEx_Erase(&flash_Rx,&SectorError) == HAL_OK)
    { 
        while(give_Data--){
            uint32_t j = 0,i;
            if(give_Data == 0 && Remain != 0)
            {
                if(Remain % 4 > 0)
                {
                    temp = Remain / 4;
                    temp++;
                } 
                else temp = Remain / 4; 
            }
            //需要干什么----->需要烧录程序---->先读满
            for(i = 0 ; i < temp;i++){
                buf[i] = *(uint32_t *)(Taddr + j);
                j += 4;
            }
            j = 0;
            //读了要干什么----->要写
            for(i = 0;i < temp;i++){
                if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,Raddr + j,buf[i]) != HAL_OK)return -2;
                j += 4;
            }
            Taddr += temp * 4;
            Raddr += temp * 4;
        }
        HAL_FLASH_Lock();
        return 0;
    }
    return -1;   //擦除失败
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
int8_t AB_Flash_Transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_BACKUP_t *T,AB_BACKUP_t *R){
    uint32_t i = 3;
    uint32_t temp = 0;
    //首先是清除计数
    BKP_Count_Clear();
    R->State = Upload_BUSY;
    AB_Backup_Flash_Write();
    if(T->len > 0x14000 || T->len < 0)return -1;
    while(i--){
        if(flash_transmit(Taddr,Raddr,erase,len,T) != 0)return -3;
        // if(T->len <= 0 || (uint32_t)T->len > 0x4000)break;
        R->len = T->len;
        if(R->len > 0x14000 || R->len < 0)break;
        temp = crc32_bitwise((const uint8_t *)Raddr,R->len);

        //算出来了接收方的CRC值，需要让这个值和接收放的进行对比，但是怎样知道传递过来的CRC在哪里
        //可以使用笨方法
        if(T->CRC32 == temp) {
            R->CRC32 = temp;
            R->State = Upload_IDLE;
            AB_Backup_Flash_Write();
            return 0;
        }
    }
    return -2;
}
