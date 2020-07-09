/*
 * (C) 2011 Pawel Ryszawa
 * Advanced Programmable Interrupt Controller (APIC) routines.
 */

#ifndef _MAIN_APIC_C
#define _MAIN_APIC_C 1

#include "../globals.h"

#ifdef CONFIG_APIC

#include "apic.h"
#include "../asm/i386.h"
#include "../asm/types.h"
#include "memory.h"
#include "cpu.h"

//extern u32 cpu_feature1;
//extern int msrsupp;
//extern ptr_t ph_addr;

char * lapic_reg = 0; /* Local APIC registers map. */
int lapic_pres = 0; /* Is Local APIC present on-chip? */

/*
 * APIC init routine.
 * We assume that "cpuid" initialization routines has already been fired.
 */
void init_apic(void)
{
  /* Check if Local APIC present: */
  if ((lapic_pres = TEST_LAPIC_PRES()))
  {
    /* If MSR supported then we can check Local APIC base address. */
    if (msrsupp)
    {
      u32 eax, edx;
      rdmsr(IA32_APIC_BASE, eax, edx);
      lapic_reg = (char *)((edx & 0xfffff000) - ph_addr); /* Local APIC register map. BTW: We rememeber our C code offset... */
    }
    else
      lapic_reg = (char *)(LAPIC_DEFAULT_BASE - ph_addr); /* Local APIC register map. BTW: We rememeber our C code offset... */
    
  }
}

#endif

#endif
