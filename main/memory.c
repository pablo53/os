/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_MEMORY_C
#define _MAIN_MEMORY_C 1


#include "../globals.h"
#include "memory.h"

#ifdef MAIN_STATIC_LINKED
#include "intr.c"
#include "lterm.c"
#include "system.c"
#else
#include "intr.h"
#include "lterm.h"
#include "system.h"
#endif
#include "../std/math_defs.h"


/*
 * This is a physical memory pointer to the beginning of this kernel's C code.
 * We expect this to be laid somewhere between 31KB and 639KB.
 */
ptr_t ph_addr = 0x00000000; /* Physical address of the beginning of the kernel's C code. */

/* Memory map structure. */
mmap_t mmap; 

/* Physical RAM limit. This is rather informational, not critical variable. */
msz_t ph_ram_limit = 0x00000000; /* To be determined... */


/*
 * This is pre-scheduler routine - for system start-up only.
 */
static int inth_sgmflt(int intno, u32 errcode, u32 segm, u32 rel_addr)
{
  tprintfk("Segmentation fault at 0x%4x:0x%8x, err=0x%8x.\n", segm, rel_addr, errcode);
  return -1; // By now, segmentation fault is critical error...
}

/*
 * Initialization of segmentation fault handling routine.
 */
void init_sgmflth(void)
{
  if (set_intr_hndl_force(13, inth_sgmflt))
  {
    tprintk("Error: Could not set up segmentation fault handler!\n");
    kpanic();
  }
}

/*
 * Initialize memory map structure.
 */
static void init_mmap(ptr_t mmap_addr, msz_t mmap_length)
{
  mmap.length = mmap_length / sizeof(addr_range_desc_t);
  mmap.rdesc = (addr_range_desc_t *)PHADDR_TO_PTR(mmap_addr);
}

/*
 * Determine RAM size.
 */
static void set_mem_limit(void)
{
  for (int i = 0; i < mmap.length; i++)
  {
    addr_range_desc_t d = mmap.rdesc[i];
    if (d.type == AD_RNG_TYP_AVAILABLE || d.type == AD_RNG_TYP_ACPI_RECL)
      ph_ram_limit = MAX(ph_ram_limit, (ptr_t)d.base_address0 + (msz_t)d.length0 - 1);
  }
}

/*
 * Init base physical address.
 */
void init_mem(ptr_t c_offset, ptr_t mmap_addr, msz_t mmap_length)
{
  ph_addr = c_offset;
  init_mmap(mmap_addr, mmap_length);
  set_mem_limit();
}

/*
 * Returns the memory map.
 */
mmap_t get_memory_map(void)
{
  return mmap;
}

/*
 * Returns the last byte of RAM (RAM size minus 1).
 */
msz_t get_mem_limit(void)
{
  return ph_ram_limit;
}


#endif
