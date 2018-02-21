#include "stm32f10x.h"
#include "flash.h"
#include "stm32f10x_flash.h"

/*
 * 函数名称：void Flash_WriteDataToFlash(uint32_t DesAddress, uint32_t* Data, uint8_t Length)
 * 输入参数：Address		写入flash的地址
 *						Data			写入的数据
 *						Length		写入数据的长度（单位为uint32_t）
 * 返回参数:
 * 函数说明：向Address地址中写入Length个uint32_t数据Data
 */
void Flash_WriteDataToFlash(uint32_t DesAddress, uint32_t* Data, uint16_t Length)
{
	vu32 Address;	
	uint32_t NbrOfPage = 0x00;
	uint32_t EraseCounter = 0x00;
	volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;	
	uint16_t i = 0 ,j = 0;
	Address = DesAddress;	
	
	FLASH_UnlockBank1();//解锁BANK1，只有解锁后才能擦除和写入
	NbrOfPage = (BANK1_WRITE_END_ADDR - BANK1_WRITE_START_ADDR) / FLASH_PAGE_SIZE;//计算一个Bank包含多少个Page
  /* Clear All pending flags */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	

/* 擦除整个Bank1 */
  for(EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
  {
    FLASHStatus = FLASH_ErasePage(BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
  }
	
//	while((Address < BANK1_WRITE_END_ADDR) && (FLASHStatus == FLASH_COMPLETE))
//	{
//					FLASHStatus = FLASH_ProgramWord(Address, Data);
//					Address = Address + 4;
//	}
	
	for(i=0; i<Length; i++)
	{
		j = 0;
		while((FLASH_COMPLETE != FLASH_ProgramWord(Address, Data[i])) && (j<10))
		{
			j++;
		}
		Address = Address + 4;
	}
	FLASH_LockBank1();//操作完成后加锁
}

 /*
 * 函数名称：void Flash_ReadDataFromFlash(uint32_t SrcAddress, uint32_t* Data, uint8_t Length);
 * 输入参数：Address		读取flash的地址
 *						Data			读取的数据
 *						Length		读取数据的长度（单位为uint32_t）
 * 返回参数：
 * 函数说明：从Address地址中读取Length个uint32_t数据Data
 */
void Flash_ReadDataFromFlash(uint32_t SrcAddress, uint32_t* Data, uint16_t Length)
{
	uint16_t i = 0;
	uint32_t address = SrcAddress;
	for(i=0; i<Length; i++)
	{
		Data[i] = (*(__IO uint32_t*) address);
		address = address+4;
	}
}
