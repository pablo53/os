/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_CPU_C
#define _MAIN_CPU_C 1

#include "../globals.h"
#include "cpu.h"

#include "../asm/types.h"
#include "../asm/i386.h"
#include "../std/string.h"

int cpuidsupp = 0; /* Is CPUID instruction supported? */
u32 maxcpuid = 0; /* Max codethat CPUID can recognize. */
char cpu_name[13] = { 0, }; /* ASCII name - as returned by CPUID:00h in EBX:EDX:ECX. */
int cpu_type = 0; /* CPU Type */
int cpu_family = 0; /* CPU Family # */
int cpu_model = 0; /* CPU Model # */
int cpu_stepid = 0; /* Stepping ID */
int cpu_brand = 0; /* Brand index */
int cpu_max_cpu = 0; /* Maximum number of addressable processor logical IDs. */
int apic_id = 0; /* Initial APIC ID */
u32 cpu_feature0 = 0; /* CPUID.01H[ECX] value. */
u32 cpu_feature1 = 0; /* CPUID.01H[EDX] value. */
int msrsupp = 0; /* Is MSR supported? (based on cpu feature 1) */

/*
 * Reads from CPUID processor instruction.
 */
void read_cpuinfo(u32 code, u32 optarg, cpuinfo_t * cpuinfo)
{
  cpuinfo->code = code;
  cpuinfo->optarg = optarg;
  if (cpuidsupp && code <= maxcpuid)
    cpuid(cpuinfo->code, cpuinfo->optarg, cpuinfo->cpuid0, cpuinfo->cpuid1, cpuinfo->cpuid2, cpuinfo->cpuid3);
  else
  {
    cpuinfo->cpuid0 = 0;
    cpuinfo->cpuid1 = 0;
    cpuinfo->cpuid2 = 0;
    cpuinfo->cpuid3 = 0;
  }
}

/*
 * Reads from MSR of the given code to edx:eax.
 */
void read_msr(u32 code, u32 * edx, u32 * eax)
{
  if (msrsupp)
    rdmsr(code, *edx, *eax);
}

/*
 * Writes to MSR of the given code from edx:eax.
 */
void write_msr(u32 code, u32 edx, u32 eax)
{
  if (msrsupp)
    wrmsr(code, edx, eax);
}

/*
 * Init routine.
 */
void init_cpu_info(void)
{
  u32 flags;
  cpuinfo_t cpuinfo;
  /* Check whether cpuid is supported: */
  get_flags(flags);
  cpuidsupp = (flags & X86_FLAG_ID);
  /* If so, read some more info from cpuids. */
  if (cpuidsupp)
  {
    /* Basic info: */
    read_cpuinfo(X86_CPUID_BASIC, 0, &cpuinfo);
    maxcpuid = cpuinfo.cpuid0;
    strncpy(&cpu_name[0], (char *)&(cpuinfo.cpuid1), 4);
    strncpy(&cpu_name[4], (char *)&(cpuinfo.cpuid3), 4);
    strncpy(&cpu_name[8], (char *)&(cpuinfo.cpuid2), 4);
    /* Version info: */
    read_cpuinfo(X86_CPUID_VERSION, 0, &cpuinfo);
    cpu_type = (cpuinfo.cpuid0 >> 12) & 0x00000003;
    cpu_family = (cpuinfo.cpuid0 >> 8) & 0x0000000f;
    if (cpu_family == 0x0000000f)
      cpu_family += (cpuinfo.cpuid0 >> 20) & 0x000000ff; /* Extended Family ID */
    cpu_model = (cpuinfo.cpuid0 >> 4) & 0x0000000f;
    if (cpu_model == 0x0000000f || cpu_model == 0x00000006)
      cpu_model += (cpuinfo.cpuid0 >> 16) & 0x0000000f; /* Extended Model ID */
    cpu_stepid = cpuinfo.cpuid0 & 0x0000000f;
    cpu_brand = cpuinfo.cpuid1 & 0x000000ff;
    cpu_max_cpu = (cpuinfo.cpuid1 >> 16) & 0x000000ff;
    apic_id = (cpuinfo.cpuid1 >> 24) & 0x000000ff;
    cpu_feature0 = (u32)cpuinfo.cpuid2;
    cpu_feature1 = (u32)cpuinfo.cpuid3;
    msrsupp = TEST_MSR_SUPPORTED();
  }
}


#endif
