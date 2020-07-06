/*
 * (C) 2011 Pawel Ryszawa
 * Kernel's physical memory allocation (i.e. "before" paging).
 */

#ifndef _MAIN_KMALLOC_H
#define _MAIN_KMALLOC_H 1

#include "../globals.h"

#include "../asm/types.h"

#define PG_SIZE 4096


void kclean_page(ptr_t); /* Pads all the page of the given physical address with zeros. */
ptr_t kget_free_page(); /* Gets a physical memory page and returns its (physical, not relative to the main C code) address. */
ptr_t kget_free_pages(u32); /* Similar to kget_free_page(), but returns a few pages of physically contiguous memory. */
int ktest_page_free(ptr_t); /* Check whether the given page is free. */
void kput_page(ptr_t); /* Returns back to pool 1 page of memory of the given physical address. */
void kput_pages(ptr_t, int); /* Returns back to pool a number of contiguous pages of memory beginning at the given physical address. */
void * kmalloc(msz_t, u16); /* Allocates memory, not less than requested, and aligned to the requested order. */
void kfree(void *); /* Frees the memory beginning at the given address. */
void init_mem_mgr(void); /* Start the whole mechanism... */


#endif
