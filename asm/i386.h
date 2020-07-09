#ifndef _ASM_I386_H
#define _ASM_I386_H 1


#define cli() __asm__ __volatile__ ("cli")
#define sti() __asm__ __volatile__ ("sti")
#define hlt() __asm__ __volatile__ ("hlt")
#define outb(port,value) __asm__ __volatile__ ("out %%al,%%dx" : : "d" (port), "a" (value))
#define outw(port,value) __asm__ __volatile__ ("out %%ax,%%dx" : : "d" (port), "a" (value))
#define outl(port,value) __asm__ __volatile__ ("out %%eax,%%dx" : : "d" (port), "a" (value))
#define inb(port,value) __asm__ __volatile__ ("in %%dx,%%al" : "=a" (value) : "d" (port))
#define inw(port,value) __asm__ __volatile__ ("in %%dx,%%ax" : "=a" (value) : "d" (port))
#define inl(port,value) __asm__ __volatile__ ("in %%dx,%%eax" : "=a" (value) : "d" (port))
#define lidt(idt48) __asm__ __volatile__ ("lidt (%%eax)" : : "a" (idt48))
#define leave() __asm__ __volatile__ ("leave")
#define iret() __asm__ __volatile__ ("iret")
#define cpuid(code,optarg,out0,out1,out2,out3) __asm__ __volatile__ ("cpuid" : "=a" (out0), "=b" (out1), "=c" (out2), "=d" (out3) : "a" (code), "c" (optarg))
#define rdmsr(code,out0,out1) __asm__ __volatile__ ("rdmsr" : "=d" (out0), "=a" (out1) : "c" (code))
#define wrmsr(code,in0,in1) __asm__ __volatile__ ("wrmsr" : : "d" (in0), "a" (in1), "c" (code))

#define get_flags(flags) __asm__ __volatile__ ( \
        "pushf \n\t" \
        "pop %%eax \n\t" \
        : "=a" (flags) \
        : \
        )

#define X86_FLAG_CF    0x00000001
#define X86_FLAG_PF    0x00000004
#define X86_FLAG_AF    0x00000010
#define X86_FLAG_ZF    0x00000040
#define X86_FLAG_SF    0x00000080
#define X86_FLAG_TF    0x00000100
#define X86_FLAG_IF    0x00000200
#define X86_FLAG_DF    0x00000400
#define X86_FLAG_OF    0x00000800
#define X86_FLAGS_IOPL 0x00003000
#define X86_FLAG_NT    0x00004000
#define X86_FLAG_RF    0x00010000
#define X86_FLAG_VM    0x00020000
#define X86_FLAG_AC    0x00040000
#define X86_FLAG_VIF   0x00080000
#define X86_FLAG_VIP   0x00100000
#define X86_FLAG_ID    0x00200000

#define X86_CPUID_BASIC            0x00000000
#define X86_CPUID_VERSION          0x00000001
#define X86_CPUID_CACHE_TLB        0x00000002
#define X86_CPUID_SERIALNO         0x00000003
#define X86_CPUID_CACHE_PARAM      0x00000004
#define X86_CPUID_MONITOR          0x00000005
#define X86_CPUID_THERMAL_POWER    0x00000006
#define X86_CPUID_DIR_CACHE_ACCESS 0x00000009
#define X86_CPUID_ARCH_PERF_MONIT  0x0000000a
#define X86_CPUID_EXT_TOPOLOGY     0x0000000b
#define X86_CPUID_EXT_STATE_ENUM   0x0000000d
#define X86_CPUID_EXTENDED_BASIC   0x80000000
#define X86_CPUID_EXTENDED_SIGN    0x80000001
#define X86_CPUID_EXTENDED_BRAND0  0x80000002
#define X86_CPUID_EXTENDED_BRAND1  0x80000003
#define X86_CPUID_EXTENDED_BRAND2  0x80000004
#define X86_CPUID_EXTENDED_CACHE   0x80000006
#define X86_CPUID_EXTENDED_TSC     0x80000007
#define X86_CPUID_EXTENDED_ADDRESS 0x80000008

#endif
