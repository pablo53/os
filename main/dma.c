/*
 * (C) 2011 Pawel Ryszawa
 * Direct Memory Access routines.
 */

#ifndef _MAIN_DMA_C
#define _MAIN_DMA_C 1

#include "../globals.h"
#include "dma.h"

#include "../asm/i386.h"
#include "../asm/types.h"
#include "system.h"

#ifdef CONFIG_DMA_8237A_AT

/* DMA 8237A's I/O ports: */
#define DMA8237A_SLAVE_ADDR0 0x0000
#define DMA8237A_SLAVE_CNT0  0x0001
#define DMA8237A_SLAVE_ADDR1 0x0002
#define DMA8237A_SLAVE_CNT1  0x0003
#define DMA8237A_SLAVE_ADDR2 0x0004
#define DMA8237A_SLAVE_CNT2  0x0005
#define DMA8237A_SLAVE_ADDR3 0x0006
#define DMA8237A_SLAVE_CNT3  0x0007
#define DMA8237A_SLAVE_STCMD 0x0008
#define DMA8237A_SLAVE_REQ   0x0009
#define DMA8237A_SLAVE_CHMSK 0x000a
#define DMA8237A_SLAVE_MODE  0x000b
#define DMA8237A_SLAVE_FFRST 0x000c
#define DMA8237A_SLAVE_BUFRG 0x000d
#define DMA8237A_SLAVE_RMASK 0x000e
#define DMA8237A_SLAVE_MASKS 0x000f
#define DMA8237A_SLAVE_PAGE2 0x0081
#define DMA8237A_SLAVE_PAGE3 0x0082
#define DMA8237A_SLAVE_PAGE1 0x0083
#define DMA8237A_SLAVE_PAGE0 0x0087
#define DMA8237A_MASTER_PAGE6 0x0089
#define DMA8237A_MASTER_PAGE7 0x008a
#define DMA8237A_MASTER_PAGE5 0x008b
#define DMA8237A_MASTER_PAGE4 0x008d
#define DMA8237A_MASTER_ADDR4 0x00c0
#define DMA8237A_MASTER_CNT4  0x00c1
#define DMA8237A_MASTER_ADDR5 0x00c2
#define DMA8237A_MASTER_CNT5  0x00c3
#define DMA8237A_MASTER_ADDR6 0x00c4
#define DMA8237A_MASTER_CNT6  0x00c5
#define DMA8237A_MASTER_ADDR7 0x00c6
#define DMA8237A_MASTER_CNT7  0x00c7
#define DMA8237A_MASTER_STCMD 0x00d0
#define DMA8237A_MASTER_REQ   0x00d2
#define DMA8237A_MASTER_CHMSK 0x00d4
#define DMA8237A_MASTER_MODE  0x00d6
#define DMA8237A_MASTER_FFRST 0x00d8
#define DMA8237A_MASTER_BUFRG 0x00da
#define DMA8237A_MASTER_RMASK 0x00dc
#define DMA8237A_MASTER_MASKS 0x00de

void reset_8237A(void)
{
  outb(DMA8237A_SLAVE_BUFRG, 0x00);  /* Value does not matter. */
  outb(DMA8237A_MASTER_BUFRG, 0x00); /* Value does not matter. */
}


/*
 * Sets address registers for a DMA controller's chanell.
 * This code is not thread safe!
 */
int set_8237A_addr(int chn, ptr_t ph_addr)
{
  /* Declarations: */
  u8 page;
  u8 offset_lo;
  u8 offset_hi;
  u16 addr_port;
  u16 page_port;
  u16 ff_port;
  /* Some checks: */
  if (chn < 0 || chn >= 8 || chn == 4)
    return DMA8237A_ERR_BAD_CHNL;
  if ((chn >= 4 && (ph_addr & 0xfffe000001)) || (chn < 4 && (ph_addr & 0xffff000000)))
    return DMA8237A_ERR_BAD_ADDR; /* Addres can be validated only after channel has been so. */
  /* Split address: */
  if (chn < 4)
    ph_addr >>= 1; /* Master DMA controller makes addresses shifted left 1 bit. */
  page = (u8)(ph_addr >> 16 & 0x000000ff);
  offset_hi = (u8)(ph_addr >> 8 & 0x000000ff);
  offset_lo = (u8)(ph_addr & 0x000000ff);
  /* Find channel's I/O ports: */
  switch (chn)
  {
    case 0:
      addr_port = DMA8237A_SLAVE_ADDR0;
      page_port = DMA8237A_SLAVE_PAGE0;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 1:
      addr_port = DMA8237A_SLAVE_ADDR1;
      page_port = DMA8237A_SLAVE_PAGE1;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 2:
      addr_port = DMA8237A_SLAVE_ADDR2;
      page_port = DMA8237A_SLAVE_PAGE2;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 3:
      addr_port = DMA8237A_SLAVE_ADDR3;
      page_port = DMA8237A_SLAVE_PAGE3;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 4:
      addr_port = DMA8237A_MASTER_ADDR4;
      page_port = DMA8237A_MASTER_PAGE4;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
    case 5:
      addr_port = DMA8237A_MASTER_ADDR5;
      page_port = DMA8237A_MASTER_PAGE5;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
    case 6:
      addr_port = DMA8237A_MASTER_ADDR6;
      page_port = DMA8237A_MASTER_PAGE6;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
    case 7:
      addr_port = DMA8237A_MASTER_ADDR7;
      page_port = DMA8237A_MASTER_PAGE7;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
  }
  /* Send to DMA: */
  outb(ff_port, 0x00); /* Reset flip flop to the "LSB" state. */
  outb(addr_port, offset_lo); /* Address - LSB */
  outb(addr_port, offset_hi); /* Address - MSB */
  outb(page_port, page); /* Page no. */
  
  return 0;
}

/*
 * Sets the counter for the given DMA channel.
 */
int set_8237A_cnt(int chn, u16 cnt)
{
  /* Declarations: */
  u8 cnt_lo;
  u8 cnt_hi;
  u16 cnt_port;
  u16 ff_port;
  /* Some checks: */
  if (chn < 0 || chn >= 8 || chn == 4)
    return DMA8237A_ERR_BAD_CHNL;
  /* Find channel's I/O ports: */
  switch (chn)
  {
    case 0:
      cnt_port = DMA8237A_SLAVE_CNT0;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 1:
      cnt_port = DMA8237A_SLAVE_CNT1;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 2:
      cnt_port = DMA8237A_SLAVE_CNT2;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 3:
      cnt_port = DMA8237A_SLAVE_CNT3;
      ff_port = DMA8237A_SLAVE_FFRST;
      break;
    case 4:
      cnt_port = DMA8237A_MASTER_CNT4;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
    case 5:
      cnt_port = DMA8237A_MASTER_CNT5;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
    case 6:
      cnt_port = DMA8237A_MASTER_CNT6;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
    case 7:
      cnt_port = DMA8237A_MASTER_CNT7;
      ff_port = DMA8237A_MASTER_FFRST;
      break;
  }
  /* Send to DMA: */
  outb(ff_port, 0x00); /* Reset flip flop to the "LSB" state. */
  outb(cnt_port, cnt_lo); /* Counter - LSB */
  outb(cnt_port, cnt_hi); /* Counter - MSB */
  
  return 0;
}

/*
 * DMA channel mode.
 * Parameters are:
 *   chn - channel (0..7)
 *   mode - mode (0..3)
 *   inc_add - increment or decrement address after each cycle (bool)
 *   rst_on_end - DMA resets itself after last cycle (bool)
 *   dir - direction (to or from memory) or "verify only" (0..3)
 */
int set_8237A_mode(int chn, int mode, int inc_addr, int rst_on_end, int dir)
{
  /* Declarations: */
  u16 mode_port;
  u8  cmd;
  /* Some checks: */
  if (chn < 0 || chn >= 8) /* Ch. #4 is allowed, however mode "cascade" should be set then. */
    return DMA8237A_ERR_BAD_CHNL;
  if (mode < 0 || mode > 4)
    return DMA8237A_ERR_BAD_MODE;
  if (dir < 0 || dir > 3)
    return DMA8237A_ERR_BAD_DIR;
  /* Find port and prepare command: */
  mode_port = (chn < 4) ? DMA8237A_SLAVE_MODE : DMA8237A_MASTER_MODE;
  cmd = (chn & 0x03) | ((mode << 6) & 0xc0) | (inc_addr ? 0 : 0x20) | (rst_on_end ? 0x10 : 0) | ((dir << 2) & 0x0c);
  /* Set mode: */
  outb(mode_port, cmd);
  return 0;
}

void init_8237A(void)
{
  reset_8237A();
}

#endif


#endif
