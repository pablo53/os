/*
 * (C) 2011 Pawel Ryszawa
 * Direct Memory Access routines.
 */

#ifndef _MAIN_DMA_H
#define _MAIN_DMA_H 1

#include "../globals.h"
#include "../asm/types.h"

#ifdef CONFIG_DMA_8237A_AT

/* Return codes: */
#define DMA8237A_ERR_BAD_ADDR 1
#define DMA8237A_ERR_BAD_CHNL 2
#define DMA8237A_ERR_BAD_MODE 3
#define DMA8237A_ERR_BAD_DIR  4

/* Mode masks: */
#define DMA8237A_MODE_DEMAND        0
#define DMA8237A_MODE_SINGLE        1
#define DMA8237A_MODE_BLOCK         2
#define DMA8237A_MODE_CASCADE       3
#define DMA8237A_MODE_ADDR_DEC      0x20
#define DMA8237A_MODE_RST_ON_END    0x10
#define DMA8237A_MODE_DIR_VERIFY    0x00
#define DMA8237A_MODE_DIR_MEM_WRITE 0x04
#define DMA8237A_MODE_DIR_MEM_READ  0x08

void init_8237A(void);
int set_8237A_addr(int, ptr_t); /* Address masked by bits 0..23 for slave, and 1..24 for master. Returns 0 on success. */
int set_8237A_cnt(int, u16); /* Setting counter for the given DMA channel. */
int set_8237A_mode(int, int, int, int, int); /* Setting modeof operation of a given DMA channel. */

#endif

#endif
