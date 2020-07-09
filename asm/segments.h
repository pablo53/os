#ifndef _ASM_SEGMENTS_H
#define _ASM_SEGMENTS_H 1


#define GDT_NULL            0x0000
#define GDT_BOOT_SEG        0x0008
#define GDT_BOOT_DATA       0x0010
#define GDT_BOOT_STACK      0x0018
#define GDT_KERNEL_BIG_SEG  0x0020
#define GDT_KERNEL_BIG_DATA 0x0028
#define GDT_KERNEL_STACK    0x0030
#define GDT_VIDEO_RAM       0x0038
#define GDT_MAIN_C_CODE     0x0040
#define GDT_MAIN_C_DATA     0x0048
#define GDT_MAIN_C_STACK    0x0050


#endif
