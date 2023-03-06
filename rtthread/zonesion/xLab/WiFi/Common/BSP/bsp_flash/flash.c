/*
***************************************Copyright (c)***************************
**                               	  �人������Ϣ�������޹�˾
**                                     		������ҵ��
**                                        
**
**                                		 
**
**--------------�ļ���Ϣ-------------------------------------------------------
**��   ��   ��: Flash.c
**��   ��   ��: ���
**����޸�����: 2018��6��4��
**��        ��: �ڲ�Flash����
**              
**--------------��ʷ�汾��Ϣ---------------------------------------------------
** ������: ���
** ��  ��: v1.0
** �ա���: 2018��6��4��
** �衡��: ��ʼ����
**
**--------------��ǰ�汾�޶�---------------------------------------------------
** �޸���: 
** �ա���: 
** �衡��: 
**
**-----------------------------------------------------------------------------
******************************************************************************/
#include  "flash.h"
#include  <string.h>

static volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

/******************************************************************************
** ��������: _flash_Page_Write
** ��������: дһҳFLASH
** ��ڲ���: pBuffer ----- ���ݻ�������WriteAddr --- д��ַ
** �� �� ֵ: ��
**
** ������: ���
** �ա���: 2022��4��28��
**-----------------------------------------------------------------------------
******************************************************************************/
static void _flash_Page_Write(uint16_t *pBuffer, uint32_t WriteAddr)
{
  uint16_t cnt_tmp;
  for(cnt_tmp=0; (cnt_tmp<FLASH_PAGE_LENGTH) && (FLASHStatus == FLASH_COMPLETE); cnt_tmp++)
  {
    FLASHStatus = FLASH_ProgramHalfWord(WriteAddr, *pBuffer);
    WriteAddr += 2;
    pBuffer++;
  }
}

/******************************************************************************
** ��������: flash_write
** ��������: ������д��flash
** ��ڲ���: Ҫд�����ʼ��ַ��Ҫд�������ͷָ�룻Ҫд������ݳ���(���ݵ�λ��16bit)
** �� �� ֵ: д������0--ʧ�ܣ�1--�ɹ�
**
** ������: ���
** �ա���: 2018��6��4��
**-----------------------------------------------------------------------------
******************************************************************************/
uint8_t flash_write(uint32_t address, uint16_t *pdata, uint16_t length) {
  static uint16_t buf_tmp[FLASH_PAGE_LENGTH];
  uint32_t NbrOfPage = 0x00;                     //ҳ������д��

  uint32_t length_beyond_start;                  //��ʼҳ�����ĳ���(����)
  uint32_t length_remain_start;                  //��ʼҳʣ��ĳ���(����)

  uint32_t addr_first_page;                      //��һҳ(��ʼ)��ַ
  uint32_t addr_last_page;                       //���ҳ(��ʼ)��ַ
  uint16_t *pbuf_tmp;                            //bufָ��
  uint16_t cnt_length;                           //���� - ����
  uint16_t cnt_page;                             //���� - ҳ��
  uint32_t prog_addr_start;                      //��̵�ַ
  uint32_t length_beyond_last;                   //���ҳ�����ĳ���(����)
  uint8_t  flag_last_page_fill;                  //���һҳװ����־

  length_beyond_start = ((address % FLASH_PAGE_SIZE) / FLASH_TYPE_LENGTH);
  length_remain_start = FLASH_PAGE_LENGTH - length_beyond_start;
  addr_first_page     = address - (address % FLASH_PAGE_SIZE);

  /* ����(д����)��ҳ�� */
  if(0 == (length_beyond_start + length)%FLASH_PAGE_LENGTH)
  {
    flag_last_page_fill = FLAG_OK;               //���һҳ�պ�
    NbrOfPage = (length_beyond_start + length) / FLASH_PAGE_LENGTH;
  }
  else
  {
    flag_last_page_fill = FLAG_NOOK;             //��������ҳ
    NbrOfPage = (length_beyond_start + length) / FLASH_PAGE_LENGTH + 1;
  }

  /* ���� */
  FLASH_UnlockBank1();

  /* �����־λ */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

  /* ������һҳ */
  flash_read(addr_first_page, buf_tmp, FLASH_PAGE_LENGTH);
  FLASHStatus = FLASH_ErasePage(addr_first_page);
  /* ֻ��1ҳ */
  if(1 == NbrOfPage)
  {
    pbuf_tmp = pdata;                          //���Ƶ�ַ(ָ��)
    for(cnt_length=length_beyond_start; cnt_length<(length_beyond_start + length); cnt_length++)
    {
      buf_tmp[cnt_length] = *pbuf_tmp;
      pbuf_tmp++;
    }
    _flash_Page_Write(buf_tmp, addr_first_page);
  }
  /* ����1ҳ */
  else
  {
    /* ��һҳ */
    pbuf_tmp = pdata;
    for(cnt_length=length_beyond_start; cnt_length<FLASH_PAGE_LENGTH; cnt_length++)
    {
      buf_tmp[cnt_length] = *pbuf_tmp;
      pbuf_tmp++;
    }
    _flash_Page_Write(buf_tmp, addr_first_page);

    /* ���һҳ�պ�װ�������ö�ȡ���һҳ���� */
    if(FLAG_OK == flag_last_page_fill)
    {
      for(cnt_page=1; (cnt_page<NbrOfPage)  && (FLASHStatus == FLASH_COMPLETE); cnt_page++)
      {                                          //�����̵�ַΪ�ֽڵ�ַ(��FLASH_PAGE_SIZE)
        prog_addr_start = addr_first_page + cnt_page*FLASH_PAGE_SIZE;
        FLASHStatus = FLASH_ErasePage(prog_addr_start);
                                                 //(cnt_page - 1):Ϊ��һҳ��ַ
        _flash_Page_Write((pdata + length_remain_start + (cnt_page - 1)*FLASH_PAGE_LENGTH), prog_addr_start);
      }
    }
    else
    {
      /* �м�ҳ */
      for(cnt_page=1; (cnt_page<(NbrOfPage - 1))  && (FLASHStatus == FLASH_COMPLETE); cnt_page++)
      {                                          //�����̵�ַΪ�ֽڵ�ַ(��FLASH_PAGE_SIZE)
        prog_addr_start = addr_first_page + cnt_page*FLASH_PAGE_SIZE;
        FLASHStatus = FLASH_ErasePage(prog_addr_start);
                                                 //(cnt_page - 1):Ϊ��һҳ��ַ
        _flash_Page_Write((pdata + length_remain_start + (cnt_page - 1)*FLASH_PAGE_LENGTH), prog_addr_start);
      }

      /* ���һҳ */
      addr_last_page = addr_first_page + (NbrOfPage - 1)*FLASH_PAGE_SIZE;

      flash_read(addr_last_page, buf_tmp, FLASH_PAGE_LENGTH);
      FLASHStatus = FLASH_ErasePage(addr_last_page);
                                                 //NbrOfPage - 2: ��ҳ + ���һҳ ����ҳ(-2)
      pbuf_tmp = pdata + length_remain_start + (NbrOfPage - 2)*(FLASH_PAGE_SIZE/2);
      length_beyond_last   = (length - length_remain_start) % FLASH_PAGE_LENGTH;
      for(cnt_length=0; cnt_length<length_beyond_last; cnt_length++)
      {
        buf_tmp[cnt_length] = *pbuf_tmp;
        pbuf_tmp++;
      }
      _flash_Page_Write(buf_tmp, addr_last_page);
    }
  }
  return 1;
}

/******************************************************************************
** ��������: flash_read
** ��������: ��ȡflash�е�����
** ��ڲ���: Ҫ��ȡ����ʼ��ַ�������ȡ�������ݵ�ͷָ�룻Ҫ��ȡ�����ݳ���(���ݵ�λ��16bit)
** �� �� ֵ: ��ȡ�����0--ʧ�ܣ�1--�ɹ�
**
** ������: ���
** �ա���: 2018��6��4��
**-----------------------------------------------------------------------------
******************************************************************************/
uint8_t flash_read(uint32_t address, uint16_t *pdata, uint16_t length) {
  if(!IS_FLASH_ADDRESS(address) || !IS_FLASH_ADDRESS(address+length) || pdata == NULL) return 0;
  
  for(uint16_t i = 0; i < length; i++) {
    *pdata++ = *(__IO uint16_t *) (address + i*2);
  }
  return 1;
}
