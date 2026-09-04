#ifndef __DAYDREAM_OTA__
#define __DAYDREAM_OTA__

#include "main.h"
#include "flash_if.h"
#include "menu.h"

#define CRC_addr        0x08008000      //CRC校验的地址     用的扇区3
#define A_addr          0x0800C000      //A目前地址         只有16KB  16384
#define B_addr          0x08010000      //B目前地址(只用作备份)         如果只使用扇区4，有64KB
#define APP_FLAG        0xf0f0f0f0
#define Buf_Num         128
#define Upload_IDLE     1
#define Upload_BUSY     2


typedef struct 
{
    uint32_t flag;         //魔数    -------------固定为APP_FLAG如果有代表后面有APP应用
    int32_t len;           //长度    -------------必须要小于最小的分区
    uint32_t CRC32;         //CRC校验码
    uint32_t State;         /*状态    
    1、IDEL   -------空闲                              
    2、BUSY   -------正在传       
    */
}AB_BACKUP_t;

typedef void (*pFunction)(void);

extern FLASH_EraseInitTypeDef flash_Rx;
extern AB_BACKUP_t A_Backup;
extern AB_BACKUP_t B_Backup;

#define BACKUP_LEN  sizeof(AB_BACKUP_t)
#define N_addr      CRC_addr




//RTC备份标志
void BKP_Count_Add(void);
//RTC备份数据清零
void BKP_Count_Clear(void);


/**
 * @brief   跳转A区
*/
void JUMP_TO_Addr(void);

/**
 * @brief   算出CRC32值
 * @param   data:数据
 * @param   length:长度
*/
uint32_t crc32_bitwise(const uint8_t *data,size_t length);





/**
 * @brief 赋值AB备份区
*/
void AB_Backup_Flash_Write(void);


/**
 * @brief 传递数据
 * @param   FlashAddress:地址
 * @param   Data:数据
 * @param   DataLength:长度

*/
void Flash_Write(uint32_t addr,AB_BACKUP_t *p);

/**
 * @brief   传数据
 * @param   Taddr:发送方
 * @param   Raddr:接收方
 * @param   erase:擦除的扇区
 * @param   len:连着擦多少扇区
 * @param   T:传过去的备份区
*/
void flash_transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_BACKUP_t *T);



/**
 * @brief   AB区互相传数据
 * @param   Taddr:发送方
 * @param   Raddr:接收方
 * @param   erase:擦除的扇区
 * @param   len:连着擦多少扇区
 * @param   T:传过去的备份区
 * @param   R:接收的的备份区
*/
void AB_Flash_Transmit(uint32_t Taddr,uint32_t Raddr,uint8_t erase,uint8_t len,AB_BACKUP_t *T,AB_BACKUP_t *R);




#endif
