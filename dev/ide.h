/*
 * (C) 2011 Pawel Ryszawa
 * IDE/ATAPI routines.
 */

#ifndef _DEV_IDE_H
#define _DEV_IDE_H 1


#include "../globals.h"


/* Primary IDE ports: */
#define IDE0_DATA_REG   0x1f0
#define IDE0_ERR_REG    0x1f1
#define IDE0_FEAT_REG   0x1f1
#define IDE0_SECT_CNT   0x1f2
#define IDE0_LBA0       0x1f3
#define IDE0_LBA1       0x1f4
#define IDE0_LBA2       0x1f5
#define IDE0_DRV_HEAD   0x1f6
#define IDE0_STATUS     0x1f7
#define IDE0_COMMAND    0x1f7
#define IDE0_ALT_STATUS 0x3f6
#define IDE0_DEV_CTL    0x3f6
#define IDE0_DRV_ADDR   0x3f7

/* Secondary IDE ports: */
#define IDE1_DATA_REG   0x170
#define IDE1_ERR_REG    0x171
#define IDE1_FEAT_REG   0x171
#define IDE1_SECT_CNT   0x172
#define IDE1_LBA0       0x173
#define IDE1_LBA1       0x174
#define IDE1_LBA2       0x175
#define IDE1_DRV_HEAD   0x176
#define IDE1_STATUS     0x177
#define IDE1_COMMAND    0x177
#define IDE1_ALT_STATUS 0x376
#define IDE1_DEV_CTL    0x376
#define IDE1_DRV_ADDR   0x377

/* Error bits (from errorregister): */
#define IDE_ERR_BBK   0x80
#define IDE_ERR_UNC   0x40
#define IDE_ERR_IDNF  0x10
#define IDE_ERR_ABRT  0x04
#define IDE_ERR_TK0NF 0x02
#define IDE_ERR_AMNF  0x01

/* Drive for "drive head" register (bits to be "or"-ed), head # mask */
#define IDE_DRV_HEAD_MASTER 0x00
#define IDE_DRV_HEAD_SLAVE  0x10
#define IDE_DRV_HEAD_HEAD_MASK 0x0f

/* Status register bits: */
#define IDE_STATUS_BUSY  0x80
#define IDE_STATUS_DRDY  0x40
#define IDE_STATUS_DWF   0x20
#define IDE_STATUS_DSC   0x10
#define IDE_STATUS_DRQ   0x08
#define IDE_STATUS_CORR  0x04
#define IDE_STATUS_INDEX 0x02
#define IDE_STATUS_ERROR 0x01

/* Commands: */
#define IDE_COMMAND_AKN_MEDIA_CHG     0xdb
#define IDE_COMMAND_POST_BOOT         0xdc
#define IDE_COMMAND_PRE_BOOT          0xdd
#define IDE_COMMAND_EXE_DRV_DIAG      0x90
#define IDE_COMMAND_FMT_TRACK         0x50
#define IDE_COMMAND_INIT_DRV_PAR      0x91
#define IDE_COMMAND_MEDIA_EJECT       0xed
#define IDE_COMMAND_NOP               0x00
#define IDE_COMMAND_READ_SECT_RTY     0x20
#define IDE_COMMAND_READ_SECT         0x21
#define IDE_COMMAND_READ_LONG_RTY     0x22
#define IDE_COMMAND_READ_LONG         0x23
#define IDE_COMMAND_READ_VER_SECT_RTY 0x40
#define IDE_COMMAND_READ_VER_SECT     0x41
#define IDE_COMMAND_RECALIBRATE       0x10
#define IDE_COMMAND_SEEK              0x70
#define IDE_COMMAND_WRITE_SECT_RTY    0x30
#define IDE_COMMAND_WRITE_SECT        0x31
#define IDE_COMMAND_WRITE_LONG_RTY    0x32
#define IDE_COMMAND_WRITE_LONG        0x33

/* Optional commands (not necessarily implemented): */
#define IDE_COMMAND_CHK_PWR_MODE    0x98
#define IDE_COMMAND_CHK_PWR_MODE_   0xe5
#define IDE_COMMAND_DOOR_LOCK       0xde
#define IDE_COMMAND_DOOR_UNLOCK     0xdf
#define IDE_COMMAND_ID_DRV          0xec
#define IDE_COMMAND_IDLE            0x97
#define IDE_COMMAND_IDLE_           0xe3
#define IDE_COMMAND_IDLE_IMM        0x95
#define IDE_COMMAND_IDLE_IMM_       0xe1
#define IDE_COMMAND_READ_BUF        0xe4
#define IDE_COMMAND_READ_DMA_RTY    0xc8
#define IDE_COMMAND_READ_DMA        0xc9
#define IDE_COMMAND_READ_MULTI      0xc4
#define IDE_COMMAND_SET_FEAT        0xef
#define IDE_COMMAND_SET_MULTI_MODE  0xc6
#define IDE_COMMAND_SET_SLEEP_MODE  0x99
#define IDE_COMMAND_SET_SLEEP_MODE_ 0xe6
#define IDE_COMMAND_STANDBY         0x96
#define IDE_COMMAND_STANDBY_        0xe2
#define IDE_COMMAND_STANDBY_IMM     0x94
#define IDE_COMMAND_STANDBY_IMM_    0xe0
#define IDE_COMMAND_WRITE_BUF       0xe8
#define IDE_COMMAND_WRITE_DMA_RTY   0xca
#define IDE_COMMAND_WRITE_DMA       0xcb
#define IDE_COMMAND_WRITE_MULTI     0xc5
#define IDE_COMMAND_WRITE_SAME      0xe9
#define IDE_COMMAND_WRITE_VER       0x3c
/* In the above definitions name final underscores gives name for the copy of the "base" command (these are doubled). */

/* ATAPI commands: */
#define IDE_COMMAND_ATAPI_PKT       0xa0
#define IDE_COMMAND_ATAPI_ID_DEV    0xa1
#define IDE_COMMAND_ATAPI_SRST      0x08

/* ATAPI optional commands: */
#define IDE_COMMAND_ATAPI_SERVICE   0xa2

/* Device control bits (bit 3 must be 1 and bit 0 must be 0 - so, always add IDE_DEV_CTL): */
#define IDE_DEV_CTL      0x08
#define IDE_DEV_CTL_SRST 0x04
#define IDE_DEV_CTL_NIEN 0x02

/* Drive address masks and bits: */
#define IDE_DRV_ADDR_NWTG 0x40
#define IDE_DRV_ADDR_NHS  0x3c
#define IDE_DRV_ADDR_NDS1 0x02
#define IDE_DRV_ADDR_NDS0 0x01

/* ATAPI operations: */

#define IDE0            0x0000
#define IDE1            0x0001
#define IDE_MASTER      0x0000
#define IDE_SLAVE       0x0010

#define IDE_MASK        0x000f
#define IDE_MTRSLV_MASK 0x00f0

#define IDEDRV_TO_IDENO(idedrv) (idedrv & IDE_MASK)
#define IDEDRV_TO_DRVNO(idedrv) ((idedrv & IDE_MTRSLV_MASK) >> 4)

#define ATAPI_MAX_PKT 16

typedef struct
{
  int tab_len; /* 12, but some devices may choose 16. */
  char tab[ATAPI_MAX_PKT]; /* Packet itself... */
} atapi_pkt_t;

/*
 * Operation routines. Each - returns error code, or 0 if no error occured.
 * Be carefull, since those functions are not thread-safe (synchronized).
 */
int atapi_send_pkt(int, atapi_pkt_t, int, ...); /* Sends ATAPI packet to the given drive. */
int atapi_get_pkt_len(int); /* Get ATAPI packet size for the given drive. 0 - if drive not present or non-ATAPI. By default it is 12 bytes, but the standard allows it to be 16. */

/* ATAPI operation flags: */
#define ATAPI_OP_F_TIMEOUT 0x0001
#define ATAPI_OP_F_SLEEP   0x0002
#define ATAPI_OP_F_CANHALT 0x0004


#endif
