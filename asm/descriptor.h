#ifndef _ASM_DESCRIPTOR_H
#define _ASM_DESCRIPTOR_H 1


#include "types.h"
#include "segments.h"


/* System segment descriptor types (withoout DPL - or DPL=0). */
#define DESC_T_TSS      0x89
#define DESC_T_LDT      0x82
#define DESC_T_TSS_BUSY 0x8b
#define DESC_T_CALLGATE 0x8c
#define DESC_T_TASKGATE 0x85
#define DESC_T_INTRGATE 0x8e
#define DESC_T_TRAPGATE 0x8f

/* Memory segment descriptor types (its lowest can change - i.e. means "segment accessed"). */
#define DESC_T_DATA_UP_RO    0x90
#define DESC_T_DATA_UP_RW    0x92
#define DESC_T_DATA_DOWN_RO  0x94
#define DESC_T_DATA_DOWN_RW  0x96
#define DESC_T_CODE_X        0x98
#define DESC_T_CODE_RX       0x9a
#define DESC_T_CODE_CONF_X   0x9c
#define DESC_T_CODE_CONF_RX  0x9e

/* DPL to be added to segment descriptor type. */
#define DESC_T_DPL0     0x00
#define DESC_T_DPL1     0x20
#define DESC_T_DPL2     0x40
#define DESC_T_DPL3     0x60

/* Granularities */
#define DESC_GRAN_1B    0x00
#define DESC_GRAN_4KB   0x80


/* Gate descriptor type */
typedef struct
{
  u16 lo_offset;
  u16 selector;
  u8  number; /* 5 lowest bites allowed only! Call gate only. */
  u8  desc_type; /* System segment descriptor type: call gate, interrupt gate or trap gate only! DPL can be added. */
  u16 hi_offset;
} gate_dsc_t;

/* Segment descriptor: memory segment or system segment. */
typedef struct
{
  u16 lo_limit;
  u16 base_addr0;
  u8  base_addr1;
  u8  desc_type; /* Segment descriptor type - except gate. Memory segment descriptors can change itself its lowest bit (bit 0). */
  u8  hi_limit; /* Bits 0..3 only, bit 4 is AVL. Granularity constant can be added (bit 7). */
  u8  base_addr2;
} segm_dsc_t;

/* Unioned descriptor type definition. */
typedef union
{
  gate_dsc_t gate;
  segm_dsc_t segm;
} dsc_t;


#define NULL_GATE_DSC() { 0x0000, 0x0000, 0x00, 0x00, 0x0000 }
#define NULL_SEGM_DSC() { 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00 }


#endif
