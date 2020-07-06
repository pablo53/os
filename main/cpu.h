/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_CPU_H
#define _MAIN_CPU_H 1

#include "../globals.h"

#include "../asm/types.h"

/* Flags returned in ECX register by CPUID.01h: */
#define CPUID_FEATURE0_SSE3        0x00000001
#define CPUID_FEATURE0_PCLMULQDQ   0x00000002
#define CPUID_FEATURE0_DTES64      0x00000004
#define CPUID_FEATURE0_MONITOR     0x00000008
#define CPUID_FEATURE0_DSCPL       0x00000010
#define CPUID_FEATURE0_VMX         0x00000020
#define CPUID_FEATURE0_SMX         0x00000040
#define CPUID_FEATURE0_EST         0x00000080
#define CPUID_FEATURE0_TM2         0x00000100
#define CPUID_FEATURE0_SSSE3       0x00000200
#define CPUID_FEATURE0_CNXTID      0x00000400
#define CPUID_FEATURE0_FMA         0x00001000
#define CPUID_FEATURE0_CMPXCHG16B  0x00002000
#define CPUID_FEATURE0_XTPR        0x00004000
#define CPUID_FEATURE0_PDCM        0x00008000
#define CPUID_FEATURE0_PCID        0x00020000
#define CPUID_FEATURE0_DCA         0x00040000
#define CPUID_FEATURE0_SSE41       0x00080000
#define CPUID_FEATURE0_SSE42       0x00100000
#define CPUID_FEATURE0_X2APIC      0x00200000
#define CPUID_FEATURE0_MOVBE       0x00400000
#define CPUID_FEATURE0_POPCNT      0x00800000
#define CPUID_FEATURE0_TSCDEADLINE 0x01000000
#define CPUID_FEATURE0_AES         0x02000000
#define CPUID_FEATURE0_XSAVE       0x04000000
#define CPUID_FEATURE0_OSXSAVE     0x08000000
#define CPUID_FEATURE0_AVX         0x10000000

/* Flags returned in EDX register by CPUID.01h: */
#define CPUID_FEATURE1_FPU   0x00000001
#define CPUID_FEATURE1_VME   0x00000002
#define CPUID_FEATURE1_DE    0x00000004
#define CPUID_FEATURE1_PSE   0x00000008
#define CPUID_FEATURE1_TSC   0x00000010
#define CPUID_FEATURE1_MSR   0x00000020
#define CPUID_FEATURE1_PAE   0x00000040
#define CPUID_FEATURE1_MCE   0x00000080
#define CPUID_FEATURE1_CX8   0x00000100
#define CPUID_FEATURE1_APIC  0x00000200
#define CPUID_FEATURE1_SEP   0x00000800
#define CPUID_FEATURE1_MTRR  0x00001000
#define CPUID_FEATURE1_PGE   0x00002000
#define CPUID_FEATURE1_MCA   0x00004000
#define CPUID_FEATURE1_CMOV  0x00008000
#define CPUID_FEATURE1_PAT   0x00010000
#define CPUID_FEATURE1_PSE36 0x00020000
#define CPUID_FEATURE1_PSN   0x00040000
#define CPUID_FEATURE1_CLFSH 0x00080000
#define CPUID_FEATURE1_DS    0x00200000
#define CPUID_FEATURE1_ACPI  0x00400000
#define CPUID_FEATURE1_MMX   0x00800000
#define CPUID_FEATURE1_FXSR  0x01000000
#define CPUID_FEATURE1_SSE   0x02000000
#define CPUID_FEATURE1_SSE2  0x04000000
#define CPUID_FEATURE1_SS    0x08000000
#define CPUID_FEATURE1_HTT   0x10000000
#define CPUID_FEATURE1_TM    0x20000000
#define CPUID_FEATURE1_PBE   0x80000000

/* Architectural model specific registers (arch. MSR) */
#define IA32_P5_MC_ADDR              0x00000000
#define IA32_P5_MC_TYPE              0x00000001
#define IA32_MONITOR_FILTER_SIZE     0x00000006
#define IA32_TIME_STAMP_COUNTER      0x00000010
#define IA32_PLATFORM_ID             0x00000017
#define IA32_APIC_BASE               0x0000001b
#define IA32_FEATURE_CONTROL         0x0000003a
#define IA32_BIOS_UPDT_TRIG          0x00000079
#define IA32_BIOS_SIGN_ID            0x0000008b
#define IA32_PMC0                    0x000000c1
#define IA32_PMC1                    0x000000c2
#define IA32_PMC2                    0x000000c3
#define IA32_PMC3                    0x000000c4
#define IA32_MPERF                   0x000000e7
#define IA32_APERF                   0x000000e8
#define IA32_MTRRCAP                 0x000000fe
#define IA32_SYSENTER_ESP            0x00000175
#define IA32_SYSENTER_EIP            0x00000176
#define IA32_MCG_CAP                 0x00000179
#define IA32_MCG_STATUS              0x0000017a
#define IA32_MCG_CTL                 0x0000017b
#define IA32_PERFEVTSEL0             0x00000186
#define IA32_PERFEVTSEL1             0x00000187
#define IA32_PERFEVTSEL2             0x00000188
#define IA32_PERFEVTSEL3             0x00000189
#define IA32_PERF_STATUS             0x00000198
#define IA32_PERF_CTL                0x00000199
#define IA32_CLOCK_MODULATION        0x0000019a
#define IA32_THERM_INTERRUPT         0x0000019b
#define IA32_THERM_STATUS            0x0000019c
#define IA32_MISC_ENABLE             0x000001a0
#define IA32_ENERGY_PERF_BIAS        0x000001b0
#define IA32_PACKAGE_THERM_STATUS    0x000001b1
#define IA32_PACKAGE_THERM_INTERRUPT 0x000001b2
#define IA32_DEBUGCTL                0x000001d9
#define IA32_SMRR_PHYSBASE           0x000001f2
#define IA32_SMRR_PHYSMASK           0x000001f3
#define IA32_PLATFORM_DCA_CAP        0x000001f8
#define IA32_CPU_DCA_CAP             0x000001f9
#define IA32_DCA_0_CAP               0x000001fa
#define IA32_MTRR_PHYSBASE0          0x00000200
#define IA32_MTRR_PHYSMASK0          0x00000201
#define IA32_MTRR_PHYSBASE1          0x00000202
#define IA32_MTRR_PHYSMASK1          0x00000203
#define IA32_MTRR_PHYSBASE2          0x00000204
#define IA32_MTRR_PHYSMASK2          0x00000205
#define IA32_MTRR_PHYSBASE3          0x00000206
#define IA32_MTRR_PHYSMASK3          0x00000207
#define IA32_MTRR_PHYSBASE4          0x00000208
#define IA32_MTRR_PHYSMASK4          0x00000209
#define IA32_MTRR_PHYSBASE5          0x0000020a
#define IA32_MTRR_PHYSMASK5          0x0000020b
#define IA32_MTRR_PHYSBASE6          0x0000020c
#define IA32_MTRR_PHYSMASK6          0x0000020d
#define IA32_MTRR_PHYSBASE7          0x0000020e
#define IA32_MTRR_PHYSMASK7          0x0000020f
#define IA32_MTRR_FIX64K_00000       0x00000250
#define IA32_MTRR_FIX16K_80000       0x00000258
#define IA32_MTRR_FIX16K_A0000       0x00000259
#define IA32_MTRR_FIX4K_C0000        0x00000268
#define IA32_MTRR_FIX4K_C8000        0x00000269
#define IA32_MTRR_FIX4K_D0000        0x0000026a
#define IA32_MTRR_FIX4K_D8000        0x0000026b
#define IA32_MTRR_FIX4K_E0000        0x0000026c
#define IA32_MTRR_FIX4K_E8000        0x0000026d
#define IA32_MTRR_FIX4K_F0000        0x0000026e
#define IA32_MTRR_FIX4K_F8000        0x0000026f
#define IA32_PAT                     0x00000277
#define IA32_MC0_CTL2                0x00000280
#define IA32_MC1_CTL2                0x00000281
#define IA32_MC2_CTL2                0x00000282
#define IA32_MC3_CTL2                0x00000283
#define IA32_MC4_CTL2                0x00000284
#define IA32_MC5_CTL2                0x00000285
#define IA32_MC6_CTL2                0x00000286
#define IA32_MC7_CTL2                0x00000287
#define IA32_MC8_CTL2                0x00000288
#define IA32_MC9_CTL2                0x00000289
#define IA32_MC10_CTL2               0x0000028a
#define IA32_MC11_CTL2               0x0000028b
#define IA32_MC12_CTL2               0x0000028c
#define IA32_MC13_CTL2               0x0000028d
#define IA32_MC14_CTL2               0x0000028e
#define IA32_MC15_CTL2               0x0000028f
#define IA32_MC16_CTL2               0x00000290
#define IA32_MC17_CTL2               0x00000291
#define IA32_MC18_CTL2               0x00000292
#define IA32_MC19_CTL2               0x00000293
#define IA32_MC20_CTL2               0x00000294
#define IA32_MC21_CTL2               0x00000295
#define IA32_MTRR_DEF_TYPE           0x000002ff
#define IA32_FIXED_CTR0              0x00000309
#define IA32_FIXED_CTR1              0x0000030a
#define IA32_FIXED_CTR2              0x0000030b
#define IA32_PERF_CAPABILITIES       0x00000345
#define IA32_FIXED_CTR_CTL           0x0000038d
#define IA32_PERF_GLOBAL_STATUS      0x0000038e
#define IA32_PERF_GLOBAL_CTRL        0x0000038f
#define IA32_PERF_GLOBAL_OVF_CTRL    0x00000390
#define IA32_PEBS_ENABLE             0x000003f1
#define IA32_MC0_CTL                 0x00000400
#define IA32_MC0_STATUS              0x00000401
#define IA32_MC0_ADDR                0x00000402
#define IA32_MC0_MISC                0x00000403
#define IA32_MC1_CTL                 0x00000404
#define IA32_MC1_STATUS              0x00000405
#define IA32_MC1_ADDR                0x00000406
#define IA32_MC1_MISC                0x00000407
#define IA32_MC2_CTL                 0x00000408
#define IA32_MC2_STATUS              0x00000409
#define IA32_MC2_ADDR                0x0000040a
#define IA32_MC2_MISC                0x0000040b
#define IA32_MC3_CTL                 0x0000040c
#define IA32_MC3_STATUS              0x0000040d
#define IA32_MC3_ADDR                0x0000040e
#define IA32_MC3_MISC                0x0000040f
#define IA32_MC4_CTL                 0x00000410
#define IA32_MC4_STATUS              0x00000411
#define IA32_MC4_ADDR                0x00000412
#define IA32_MC4_MISC                0x00000413
#define IA32_MC5_CTL                 0x00000414
#define IA32_MC5_STATUS              0x00000415
#define IA32_MC5_ADDR                0x00000416
#define IA32_MC5_MISC                0x00000417
#define IA32_MC6_CTL                 0x00000418
#define IA32_MC6_STATUS              0x00000419
#define IA32_MC6_ADDR                0x0000041a
#define IA32_MC6_MISC                0x0000041b
#define IA32_MC7_CTL                 0x0000041c
#define IA32_MC7_STATUS              0x0000041d
#define IA32_MC7_ADDR                0x0000041e
#define IA32_MC7_MISC                0x0000041f
#define IA32_MC8_CTL                 0x00000420
#define IA32_MC8_STATUS              0x00000421
#define IA32_MC8_ADDR                0x00000422
#define IA32_MC8_MISC                0x00000423
#define IA32_MC9_CTL                 0x00000424
#define IA32_MC9_STATUS              0x00000425
#define IA32_MC9_ADDR                0x00000426
#define IA32_MC9_MISC                0x00000427
#define IA32_MC10_CTL                0x00000428
#define IA32_MC10_STATUS             0x00000429
#define IA32_MC10_ADDR               0x0000042a
#define IA32_MC10_MISC               0x0000042b
#define IA32_MC11_CTL                0x0000042c
#define IA32_MC11_STATUS             0x0000042d
#define IA32_MC11_ADDR               0x0000042e
#define IA32_MC11_MISC               0x0000042f
#define IA32_MC12_CTL                0x00000430
#define IA32_MC12_STATUS             0x00000431
#define IA32_MC12_ADDR               0x00000432
#define IA32_MC12_MISC               0x00000433
#define IA32_MC13_CTL                0x00000434
#define IA32_MC13_STATUS             0x00000435
#define IA32_MC13_ADDR               0x00000436
#define IA32_MC13_MISC               0x00000437
#define IA32_MC14_CTL                0x00000438
#define IA32_MC14_STATUS             0x00000439
#define IA32_MC14_ADDR               0x0000043a
#define IA32_MC14_MISC               0x0000043b
#define IA32_MC15_CTL                0x0000043c
#define IA32_MC15_STATUS             0x0000043d
#define IA32_MC15_ADDR               0x0000043e
#define IA32_MC15_MISC               0x0000043f
#define IA32_MC16_CTL                0x00000440
#define IA32_MC16_STATUS             0x00000441
#define IA32_MC16_ADDR               0x00000442
#define IA32_MC16_MISC               0x00000443
#define IA32_MC17_CTL                0x00000444
#define IA32_MC17_STATUS             0x00000445
#define IA32_MC17_ADDR               0x00000446
#define IA32_MC17_MISC               0x00000447
#define IA32_MC18_CTL                0x00000448
#define IA32_MC18_STATUS             0x00000449
#define IA32_MC18_ADDR               0x0000044a
#define IA32_MC18_MISC               0x0000044b
#define IA32_MC19_CTL                0x0000044c
#define IA32_MC19_STATUS             0x0000044d
#define IA32_MC19_ADDR               0x0000044e
#define IA32_MC19_MISC               0x0000044f
#define IA32_MC20_CTL                0x00000450
#define IA32_MC20_STATUS             0x00000451
#define IA32_MC20_ADDR               0x00000452
#define IA32_MC20_MISC               0x00000453
#define IA32_MC21_CTL                0x00000454
#define IA32_MC21_STATUS             0x00000455
#define IA32_MC21_ADDR               0x00000456
#define IA32_MC21_MISC               0x00000457
#define IA32_VMX_BASIC               0x00000480
#define IA32_VMX_PINBASED_CTLS       0x00000481
#define IA32_VMX_PROCBASED_CTLS      0x00000482
#define IA32_VMX_EXIT_CTLS           0x00000483
#define IA32_VMX_ENTRY_CTLS          0x00000484
#define IA32_VMX_MISC                0x00000485
#define IA32_VMX_CR0_FIXED0          0x00000486
#define IA32_VMX_CR0_FIXED1          0x00000487
#define IA32_VMX_CR4_FIXED0          0x00000488
#define IA32_VMX_CR4_FIXED1          0x00000489
#define IA32_VMX_VMCS_ENUM           0x0000048a
#define IA32_VMX_PROCBASED_CTLS2     0x0000048b
#define IA32_VMX_EPT_VPID_CAP        0x0000048c
#define IA32_VMX_TRUE_PINBASED_CTLS  0x0000048d
#define IA32_VMX_TRUE_PROCBASED_CTLS 0x0000048e
#define IA32_VMX_TRUE_EXIT_CTLS      0x0000048f
#define IA32_VMX_TRUE_ENTRY_CTLS     0x00000490
#define IA32_DS_AREA                 0x00000600
#define IA32_TSC_DEADLINE            0x000006e0
#define IA32_X2APIC_APICID           0x00000802
#define IA32_X2APIC_VERSION          0x00000803
#define IA32_X2APIC_TPR              0x00000808
#define IA32_X2APIC_PPR              0x0000080a
#define IA32_X2APIC_EOI              0x0000080b
#define IA32_X2APIC_LDR              0x0000080d
#define IA32_X2APIC_SIVR             0x0000080f
#define IA32_X2APIC_ISR0             0x00000810
#define IA32_X2APIC_ISR1             0x00000811
#define IA32_X2APIC_ISR2             0x00000812
#define IA32_X2APIC_ISR3             0x00000813
#define IA32_X2APIC_ISR4             0x00000814
#define IA32_X2APIC_ISR5             0x00000815
#define IA32_X2APIC_ISR6             0x00000816
#define IA32_X2APIC_ISR7             0x00000817
#define IA32_X2APIC_TMR0             0x00000818
#define IA32_X2APIC_TMR1             0x00000819
#define IA32_X2APIC_TMR2             0x0000081a
#define IA32_X2APIC_TMR3             0x0000081b
#define IA32_X2APIC_TMR4             0x0000081c
#define IA32_X2APIC_TMR5             0x0000081d
#define IA32_X2APIC_TMR6             0x0000081e
#define IA32_X2APIC_TMR7             0x0000081f
#define IA32_X2APIC_IRR0             0x00000820
#define IA32_X2APIC_IRR1             0x00000821
#define IA32_X2APIC_IRR2             0x00000822
#define IA32_X2APIC_IRR3             0x00000823
#define IA32_X2APIC_IRR4             0x00000824
#define IA32_X2APIC_IRR5             0x00000825
#define IA32_X2APIC_IRR6             0x00000826
#define IA32_X2APIC_IRR7             0x00000827
#define IA32_X2APIC_ESR              0x00000828
#define IA32_X2APIC_LVT_CMCI         0x0000082f
#define IA32_X2APIC_ICR              0x00000830
#define IA32_X2APIC_LVT_TIMER        0x00000832
#define IA32_X2APIC_LVT_THERMAL      0x00000833
#define IA32_X2APIC_LVT_PMI          0x00000834
#define IA32_X2APIC_LVT_LINT0        0x00000835
#define IA32_X2APIC_LVT_LINT1        0x00000836
#define IA32_X2APIC_LVT_ERROR        0x00000837
#define IA32_X2APIC_LVT_COUNT        0x00000838
#define IA32_X2APIC_CUR_COUNT        0x00000839
#define IA32_X2APIC_DIV_CONF         0x0000083e
#define IA32_X2APIC_SELF_IPI         0x0000083f
#define IA32_EFER                    0xc0000080
#define IA32_STAR                    0xc0000081
#define IA32_LSTAR                   0xc0000082
#define IA32_FMASK                   0xc0000084
#define IA32_FS_BASE                 0xc0000100
#define IA32_GS_BASE                 0xc0000101
#define IA32_KERNEL_GS_BASE          0xc0000102
#define IA32_TSC_AUX                 0xc0000103


typedef struct
{
  u32 code; /* EAX passed to CPUID */
  u32 optarg; /* ECX passed to CPUID */
  u32 cpuid0; /* EAX returned by CPUID. */
  u32 cpuid1; /* EBX returned by CPUID. */
  u32 cpuid2; /* ECX returned by CPUID. */
  u32 cpuid3; /* EDX returned by CPUID. */
} cpuinfo_t;

void read_cpuinfo(u32, u32, cpuinfo_t *);
void read_msr(u32, u32 *, u32 *);
void write_msr(u32, u32, u32);

void init_cpu_info(void);

#define TEST_MSR_SUPPORTED() (cpu_feature1 & CPUID_FEATURE1_MSR)
#define TEST_CPU_COMPATIBILITY(family,model,stepid) ((family) > cpu_family || ((family) == cpu_family && (model) > cpu_model) || ((family) == cpu_family && (model) == cpu_model && (stepid) >= cpu_stepid))


#endif
