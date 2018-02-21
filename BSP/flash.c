#include "stm32f10x.h"
#include "flash.h"
#include "stm32f10x_flash.h"

/*
 * �������ƣ�void Flash_WriteDataToFlash(uint32_t DesAddress, uint32_t* Data, uint8_t Length)
 * ���������Address		д��flash�ĵ�ַ
 *						Data			д�������
 *						Length		д�����ݵĳ��ȣ���λΪuint32_t��
 * ���ز���:
 * ����˵������Address��ַ��д��Length��uint32_t����Data
 */
void Flash_WriteDataToFlash(uint32_t DesAddress, uint32_t* Data, uint16_t Length)
{
	vu32 Address;	
	uint32_t NbrOfPage = 0x00;
	uint32_t EraseCounter = 0x00;
	volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;	
	uint16_t i = 0 ,j = 0;
	Address = DesAddress;	
	
	FLASH_UnlockBank1();//����BANK1��ֻ�н�������ܲ�����д��
	NbrOfPage = (BANK1_WRITE_END_ADDR - BANK1_WRITE_START_ADDR) / FLASH_PAGE_SIZE;//����һ��Bank�������ٸ�Page
  /* Clear All pending flags */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	

/* ��������Bank1 */
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
	FLASH_LockBank1();//������ɺ����
}

 /*
 * �������ƣ�void Flash_ReadDataFromFlash(uint32_t SrcAddress, uint32_t* Data, uint8_t Length);
 * ���������Address		��ȡflash�ĵ�ַ
 *						Data			��ȡ������
 *						Length		��ȡ���ݵĳ��ȣ���λΪuint32_t��
 * ���ز�����
 * ����˵������Address��ַ�ж�ȡLength��uint32_t����Data
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
