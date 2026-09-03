#ifndef __DAYDREAM_OTA__
#define __DAYDREAM_OTA__

#include "main.h"
#include "flash_if.h"
#include "menu.h"

#define CRC_addr        0x08008000      //CRC校验的地址     用的扇区3
#define A_addr          0x0800C000      //A目前地址         只有16KB  16384
#define B_addr          0x08010000      //B目前地址(只用作备份)         如果只使用扇区4，有64KB
#define Buf_Num         128
#define Upload_IDLE     1
#define Upload_READY    2
#define Upload_BUSY     3


typedef struct 
{
    uint32_t Magic;         //魔数    -------------固定为0x12345678如果有代表后面有APP应用
    int32_t len;           //长度    -------------必须要小于最小的分区
    uint32_t CRC32;         //CRC校验码
    uint32_t State;         /*状态    
    1、IDEL   -------空闲                              
    2、READY  -------?就绪
    3、BUSY   -------正在传       
    */
}AB_CRC;


FLASH_EraseInitTypeDef flash_Rx;
pFunction Jump_To_App;
pFunction Reset_Addr;
AB_CRC Last_CRC;
AB_CRC New_CRC;




//跳去A
void JUMP_TO_Addr();

//AB分区互相传数据
void flash_transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_CRC *C);

//CRC32校验
uint32_t crc32_bitwise(const uint8_t *data,size_t length);

void BKP_Count_Add(void);
void BKP_Count_Clear(void);






#endif
