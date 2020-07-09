/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_MEMORY_H
#define _MAIN_MEMORY_H 1


#include "../globals.h"
#include "../asm/types.h"


extern ptr_t ph_addr; /* Physical address of the beginning of the kernel's C code. */

void init_sgmflth(void);
void init_mem(ptr_t, ptr_t, msz_t);

/* Convertion between C code pointers and explicit physical addresses: */
#define PTR_TO_PHADDR(p) (((ptr_t)((void *)p)) + ph_addr)
#define PHADDR_TO_PTR(a) ((void *)((a) - ph_addr))
#define PHADDR_TO_CADDR(a) ((a) - ph_addr)
#define CADDR_TO_PHADDR(a) ((a) + ph_addr)
#define PTR_ADD_BYTES(p,x) ((void *)(((ptr_t)((void *)(p))) + (x)))

/* Address range descriptor type - as returned by BIOS INT 0x15, function 0xe820 ("get memory map"). */
typedef struct 
{
  u32 base_address0; /* Lower 32 bits of base address */
  u32 base_address1; /* Upper 32 bits of base address (beyond the 32-bit scope!) */
  u32 length0;       /* Lower 32 bits of the length. */
  u32 length1;       /* Upper 32 bits of the length (beyond the 32-bit scope!). */
  u16 type;          /* The type of the range. */
  u16 ext_attr ;     /* Extended attributes. */
} addr_range_desc_t;

#define AD_RNG_TYP_AVAILABLE 1
#define AD_RNG_TYP_RESERVED  2
#define AD_RNG_TYP_ACPI_RECL 3
#define AD_RNG_TYP_ACPI_NVS  4
#define AD_RNG_TYP_UNUSEABLE 5
#define AD_RNG_TYP_DISABLED  6

typedef struct
{
  int length; /* Number of address range descriptors. */
  addr_range_desc_t * rdesc; /* Table of address range descriptors. */
} mmap_t;

mmap_t get_memory_map(void); /* Returns the memory map - as declared by BIOS during the booting procedure. */
msz_t get_mem_limit(void); /* Returns the last RAM address determined at boot time. */

#endif
