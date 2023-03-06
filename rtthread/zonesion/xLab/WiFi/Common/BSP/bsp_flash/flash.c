/*
***************************************Copyright (c)***************************
**                               	  武汉钛联信息技术有限公司
**                                     		工程事业部
**                                        
**
**                                		 
**
**--------------文件信息-------------------------------------------------------
**文   件   名: Flash.c
**创   建   人: 杨开琦
**最后修改日期: 2018年6月4日
**描        述: 内部Flash操作
**              
**--------------历史版本信息---------------------------------------------------
** 创建人: 杨开琦
** 版  本: v1.0
** 日　期: 2018年6月4日
** 描　述: 初始创建
**
**--------------当前版本修订---------------------------------------------------
** 修改人: 
** 日　期: 
** 描　述: 
**
**-----------------------------------------------------------------------------
******************************************************************************/
#include  "flash.h"
#include  <string.h>

static volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

/******************************************************************************
** 函数名称: _flash_Page_Write
** 功能描述: 写一页FLASH
** 入口参数: pBuffer ----- 数据缓冲区；WriteAddr --- 写地址
** 返 回 值: 无
**
** 作　者: 杨开琦
** 日　期: 2022年4月28日
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
** 函数名称: flash_write
** 功能描述: 将数据写入flash
** 入口参数: 要写入的起始地址；要写入的数据头指针；要写入的数据长度(数据单位：16bit)
** 返 回 值: 写入结果：0--失败；1--成功
**
** 作　者: 杨开琦
** 日　期: 2018年6月4日
**-----------------------------------------------------------------------------
******************************************************************************/
uint8_t flash_write(uint32_t address, uint16_t *pdata, uint16_t length) {
  static uint16_t buf_tmp[FLASH_PAGE_LENGTH];
  uint32_t NbrOfPage = 0x00;                     //页数（读写）

  uint32_t length_beyond_start;                  //开始页超出的长度(半字)
  uint32_t length_remain_start;                  //开始页剩余的长度(半字)

  uint32_t addr_first_page;                      //第一页(起始)地址
  uint32_t addr_last_page;                       //最后页(起始)地址
  uint16_t *pbuf_tmp;                            //buf指针
  uint16_t cnt_length;                           //计数 - 长度
  uint16_t cnt_page;                             //计数 - 页数
  uint32_t prog_addr_start;                      //编程地址
  uint32_t length_beyond_last;                   //最后页超出的长度(半字)
  uint8_t  flag_last_page_fill;                  //最后一页装满标志

  length_beyond_start = ((address % FLASH_PAGE_SIZE) / FLASH_TYPE_LENGTH);
  length_remain_start = FLASH_PAGE_LENGTH - length_beyond_start;
  addr_first_page     = address - (address % FLASH_PAGE_SIZE);

  /* 擦除(写操作)的页数 */
  if(0 == (length_beyond_start + length)%FLASH_PAGE_LENGTH)
  {
    flag_last_page_fill = FLAG_OK;               //最后一页刚好
    NbrOfPage = (length_beyond_start + length) / FLASH_PAGE_LENGTH;
  }
  else
  {
    flag_last_page_fill = FLAG_NOOK;             //···跨页
    NbrOfPage = (length_beyond_start + length) / FLASH_PAGE_LENGTH + 1;
  }

  /* 解锁 */
  FLASH_UnlockBank1();

  /* 清除标志位 */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

  /* 操作第一页 */
  flash_read(addr_first_page, buf_tmp, FLASH_PAGE_LENGTH);
  FLASHStatus = FLASH_ErasePage(addr_first_page);
  /* 只有1页 */
  if(1 == NbrOfPage)
  {
    pbuf_tmp = pdata;                          //复制地址(指针)
    for(cnt_length=length_beyond_start; cnt_length<(length_beyond_start + length); cnt_length++)
    {
      buf_tmp[cnt_length] = *pbuf_tmp;
      pbuf_tmp++;
    }
    _flash_Page_Write(buf_tmp, addr_first_page);
  }
  /* 大于1页 */
  else
  {
    /* 第一页 */
    pbuf_tmp = pdata;
    for(cnt_length=length_beyond_start; cnt_length<FLASH_PAGE_LENGTH; cnt_length++)
    {
      buf_tmp[cnt_length] = *pbuf_tmp;
      pbuf_tmp++;
    }
    _flash_Page_Write(buf_tmp, addr_first_page);

    /* 最后一页刚好装满，不用读取最后一页数据 */
    if(FLAG_OK == flag_last_page_fill)
    {
      for(cnt_page=1; (cnt_page<NbrOfPage)  && (FLASHStatus == FLASH_COMPLETE); cnt_page++)
      {                                          //这里编程地址为字节地址(故FLASH_PAGE_SIZE)
        prog_addr_start = addr_first_page + cnt_page*FLASH_PAGE_SIZE;
        FLASHStatus = FLASH_ErasePage(prog_addr_start);
                                                 //(cnt_page - 1):为下一页地址
        _flash_Page_Write((pdata + length_remain_start + (cnt_page - 1)*FLASH_PAGE_LENGTH), prog_addr_start);
      }
    }
    else
    {
      /* 中间页 */
      for(cnt_page=1; (cnt_page<(NbrOfPage - 1))  && (FLASHStatus == FLASH_COMPLETE); cnt_page++)
      {                                          //这里编程地址为字节地址(故FLASH_PAGE_SIZE)
        prog_addr_start = addr_first_page + cnt_page*FLASH_PAGE_SIZE;
        FLASHStatus = FLASH_ErasePage(prog_addr_start);
                                                 //(cnt_page - 1):为下一页地址
        _flash_Page_Write((pdata + length_remain_start + (cnt_page - 1)*FLASH_PAGE_LENGTH), prog_addr_start);
      }

      /* 最后一页 */
      addr_last_page = addr_first_page + (NbrOfPage - 1)*FLASH_PAGE_SIZE;

      flash_read(addr_last_page, buf_tmp, FLASH_PAGE_LENGTH);
      FLASHStatus = FLASH_ErasePage(addr_last_page);
                                                 //NbrOfPage - 2: 首页 + 最后一页 共两页(-2)
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
** 函数名称: flash_read
** 功能描述: 读取flash中的数据
** 入口参数: 要读取的起始地址；保存读取出的数据的头指针；要读取的数据长度(数据单位：16bit)
** 返 回 值: 读取结果：0--失败；1--成功
**
** 作　者: 杨开琦
** 日　期: 2018年6月4日
**-----------------------------------------------------------------------------
******************************************************************************/
uint8_t flash_read(uint32_t address, uint16_t *pdata, uint16_t length) {
  if(!IS_FLASH_ADDRESS(address) || !IS_FLASH_ADDRESS(address+length) || pdata == NULL) return 0;
  
  for(uint16_t i = 0; i < length; i++) {
    *pdata++ = *(__IO uint16_t *) (address + i*2);
  }
  return 1;
}
