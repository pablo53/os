/*
 * (C) 2011 Pawel Ryszawa
 * Advanced Programmable Interrupt Controller (APIC) routines.
 */

#ifndef _MAIN_APIC_H
#define _MAIN_APIC_H 1

#include "../globals.h"

#ifdef CONFIG_APIC


/* Physical memory address base for Local APIC area: */
#define LAPIC_DEFAULT_BASE 0xfee00000
/* Offsets to the base address Local APIC for registers: */
#define LAPIC_REG_ID          0x0020
#define LAPIC_REG_VER         0x0030
#define LAPIC_REG_TASK_PRIOR  0x0080
#define LAPIC_REG_ARBITR_PR   0x0090
#define LAPIC_REG_PROC_PRIOR  0x00a0
#define LAPIC_REG_EOI         0x00b0
#define LAPIC_REG_REM_READ    0x00c0
#define LAPIC_REG_LOC_DEST    0x00d0
#define LAPIC_REG_DEST_FMT    0x00e0
#define LAPIC_REG_SPUR_VEC    0x00f0
#define LAPIC_REG_IN_SERVICE0 0x0100
#define LAPIC_REG_IN_SERVICE1 0x0110
#define LAPIC_REG_IN_SERVICE2 0x0120
#define LAPIC_REG_IN_SERVICE3 0x0130
#define LAPIC_REG_IN_SERVICE4 0x0140
#define LAPIC_REG_IN_SERVICE5 0x0150
#define LAPIC_REG_IN_SERVICE6 0x0160
#define LAPIC_REG_IN_SERVICE7 0x0170
#define LAPIC_REG_TRG_MODE0   0x0180
#define LAPIC_REG_TRG_MODE1   0x0190
#define LAPIC_REG_TRG_MODE2   0x01a0
#define LAPIC_REG_TRG_MODE3   0x01b0
#define LAPIC_REG_TRG_MODE4   0x01c0
#define LAPIC_REG_TRG_MODE5   0x01d0
#define LAPIC_REG_TRG_MODE6   0x01e0
#define LAPIC_REG_TRG_MODE7   0x01f0
#define LAPIC_REG_INTR_REQ0   0x0200
#define LAPIC_REG_INTR_REQ1   0x0210
#define LAPIC_REG_INTR_REQ2   0x0220
#define LAPIC_REG_INTR_REQ3   0x0230
#define LAPIC_REG_INTR_REQ4   0x0240
#define LAPIC_REG_INTR_REQ5   0x0250
#define LAPIC_REG_INTR_REQ6   0x0260
#define LAPIC_REG_INTR_REQ7   0x0270
#define LAPIC_REG_ERR_STAT    0x0280
#define LAPIC_REG_LVT_CMCI    0x02f0
#define LAPIC_REG_INTR_CMD0   0x0300
#define LAPIC_REG_INTR_CMD1   0x0310
#define LAPIC_REG_LVT_TIMER   0x0320
#define LAPIC_REG_LVT_THERM_S 0x0330
#define LAPIC_REG_LVT_PERF_MN 0x0340
#define LAPIC_REG_LVT_LINT0   0x0350
#define LAPIC_REG_LVT_LINT1   0x0360
#define LAPIC_REG_LVT_ERR     0x0370
#define LAPIC_REG_INIT_CNT    0x0380
#define LAPIC_REG_CURRENT_CNT 0x0390
#define LAPIC_REG_DIV_CONFIG  0x03e0

#define TEST_LAPIC_PRES() (cpu_feature1 & CPUID_FEATURE1_APIC)

extern char * lapic_reg; /* Local APIC registers map. */
extern int lapic_pres; /* Is Local APIC present on-chip? */
extern void init_apic(void);


#endif

#endif
