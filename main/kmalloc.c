/*
 * (C) 2011 Pawel Ryszawa
 * Kernel's physical memory allocation (i.e. "before" paging).
 */

#ifndef _MAIN_KMALLOC_C
#define _MAIN_KMALLOC_C 1

#include "../globals.h"

#include "kmalloc.h"

#include "../asm/types.h"
#include "../asm/segments.h"
#include "err.h"
#ifdef MAIN_STATIC_LINKED
#include "memory.c"
#include "system.c"
#include "lterm.c"
#include "../std/math.c"
#else
#include "memory.h"
#include "system.h"
#include "lterm.h"
#include "../std/math.h"
#endif

#ifndef MAIN_STATIC_LINKED
extern ptr_t ph_addr;
#endif

#define PG_BITMAPS 1024
#define PG_BITMAP_SIZE 128
/* The following are bit shifts and masks to derive some information from the physical memory address. */
#define PG_BMAPNO_SHFT 22
#define PG_BMAPNO_MASK 0xffc00000
#define PG_BYTEIDX_SHFT 15
#define PG_BYTEIDX_MASK 0x003f8000
#define PG_BITIDX_SHFT 12
#define PG_BITIDX_MASK 0x00007000
#define PG_OFFSET 0x00000fff

#define PGADDR_TO_BMAPNO(pg_addr) ((int)((((ptr_t)pg_addr) & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT))

/* Some parameters for allocationg "memory slots" (i.e. memory areas smaller than a page). */
#define MEMSLOT_MIN_SHFT 6
#define MEMSLOT_MAX_SHFT (PG_BITIDX_SHFT - 1)
#define MEMSLOT_SHFT_RANGE (PG_BITIDX_SHFT - MEMSLOT_MIN_SHFT)
#define MEMSLOT_MIN (1 << MEMSLOT_MIN_SHFT)
#define MEMSLOT_MIN_OFF_MASK (MEMSLOT_MIN - 1)
#define MEMSLOT_PTR_PER_BLK 1024
#define MEMSLOT_PTR_PREV 0
#define MEMSLOT_PTR_NEXT 1
#define MEMSLOT_PTR_MIN 2
#define MEMSLOT_PTR_MAX 1023
#define MEMSLOT_MAX_ALIGN 512

#define MSLOT_SIZE(order) (1 << (order))
#define MSLOT_PTR_PREV(slotblk) ((*((slotblk_t *)(slotblk)))[MEMSLOT_PTR_PREV])
#define MSLOT_PTR_NEXT(slotblk) ((*((slotblk_t *)(slotblk)))[MEMSLOT_PTR_NEXT])
#define MSLOT_PTR_CURRENT(order) bulkslot[(order) - MEMSLOT_MIN_SHFT]

typedef void * slotblk_t[MEMSLOT_PTR_PER_BLK]; /* Table of pointers. Its size must conform to page size! */

/*
 * Array of pointers to pages that start the list of free slots. Each pointer
 * correspond to a list of slots of different size (from 1 << MEMSLOT_MIN_SHFT
 * to 1 << MEMSLOT_MAX_SHFT).
 * Be carefull: these are C-code-relative pointers, not physical addresses.
 * Each pointer points to the beginning of a page. Each page consists of 1024
 * 32-bit C-code-relative pointers (1024 bytes x 32 bit = 4096 bytes = page size).
 * In every page, the first pointer points to the previous page (or is null, if
 * this is a first page in the list). Another one points to the next page of a
 * list (or is null if this is the last pointer in the last page of the list).
 * 1022 remaining pointers points to the beginning of its corresponding free area
 * Pointers are sorted by its value. The length of the area depends on the list the
 * pointer belongs to. Each of the MEMSLOT_SHFT_RANGE lists relates to a different
 * size, ranging from 64 (= 2^5) to 2048 (= 2^11). A pointer of value 0 is NULL, and
 * as such does not point anything.
 */
static slotblk_t * bulkslot[MEMSLOT_SHFT_RANGE] = { 0, }; /* Table of pointers to pages containing another level of table of pointers.  */

/*
 * Some flags to modyfy functions' behavior.
 */
#define MEMSLOT_F_NO_FLAGS    0
#define MEMSLOT_F_EXACT_MATCH 1
#define MEMSLOT_F_LOCK_BLOCK  2
#define MEMSLOT_F_INCLUSIVELY 4

/*
 * Finds the slot (if exists) in the list of the given order.
 * Sets corresponding bulkslot entry to point the block in which,
 * the slot was found. Returns index in the block that contains
 * the slot being found.
 * It is necessary to check out whether the routine actually succeeded,
 * since the slot may not be present in the list. If the slot was
 * missing either ERR_NOT_FOUND or an index to the first slot behind the
 * one being looked for is returned. If there are no slots behind, and
 * all the pointers in the list are used (i.e. all are non-nulls),
 * then the pointer returned exceeds the maximum allowed for the
 * block (points the the non existing one item behind the actual
 * last one in the last block) - this indicates that a new block
 * must be allocated in order to put another slot to the list.
 * Be carefull about the flags. If you lock the block, return value
 * not necessarily will correspond to the block you find "current"!
 */
static int kfind_mslot(void * slot, int order, int mode_flags)
{
  /* Decl. */
  slotblk_t * blk; /* Auxiliary pointer: current block. */
  /* First check whether the list of the given order exists. */
  if (order < MEMSLOT_MIN_SHFT || order > MEMSLOT_MAX_SHFT)
    return ERR_OUT_OF_BOUND;
  else if (!(blk = MSLOT_PTR_CURRENT(order)))
    return ERR_NOT_FOUND;
  /* Seek */
  int inblk = MEMSLOT_PTR_MIN; /* Intra-block index. */
  int rmvd = 0; /* A flag that we moved on to the next block at least once. */
  for (;;)
  {
    /* First, check if we have found what we are looking for. */
    if (PTR_TO_PHADDR((*blk)[inblk]) == PTR_TO_PHADDR(slot))
      return inblk;
    else if (PTR_TO_PHADDR((*blk)[inblk]) > PTR_TO_PHADDR(slot) || !((*blk)[inblk]))
    {
      if (!rmvd && MSLOT_PTR_PREV(blk) && inblk == MEMSLOT_PTR_MIN)
      {
        blk = MSLOT_PTR_PREV(blk); /* Move on to the previous block. */
        if (!(mode_flags & MEMSLOT_F_LOCK_BLOCK))
          MSLOT_PTR_CURRENT(order) = blk;
      }
      else
        return (mode_flags & MEMSLOT_F_EXACT_MATCH) ? ERR_NOT_FOUND : inblk; /* So... the slot is not present in the list. */
    }
    else
    {
      /* Next slot (check if this was last in the block): */
      if (++inblk > MEMSLOT_PTR_MAX)
      {
        if (MSLOT_PTR_NEXT(blk))
        {
          /* Move on to the next block: */
          blk = MSLOT_PTR_PREV(blk);
          if (!(mode_flags & MEMSLOT_F_LOCK_BLOCK))
            MSLOT_PTR_CURRENT(order) = blk;
          inblk = MEMSLOT_PTR_MIN;
        }
        else
          return (mode_flags & MEMSLOT_F_EXACT_MATCH) ? ERR_NOT_FOUND : inblk; /* So... the slot is not present in the list and the last block has been reached. */
      }
      rmvd = 1; /* Since now, we cannot make any moves "left" (i.e. to a previous block). */
    }
  }
}

/*
 * Inserts slot at the given position. It is caller's responsibility to determine the
 * right place to do this. The slot's size order (as a power of 2) is not important here
 * because a pointer to the right block is passed - so referencing to this block via
 * "bulkslot" would be redundant.
 * This function return 0 on success or error (e.g. "out of memory").
 * (This routine is for other routines operating on memory slots.)
 */
static int kinsert_mslot(void * slot, slotblk_t * blk, int inblk)
{
  /* Check constraints: */
  if (inblk < MEMSLOT_PTR_MIN || inblk > MEMSLOT_PTR_MAX)
    return ERR_OUT_OF_BOUND;
  else if (!blk)
    return ERR_BAD_PARAMETER;
  /* Put the slot here, and move one position forward */
  void * old_slot = (*blk)[inblk];
  while (slot != (void *)0) /* Slot parameter is going to be reused as an ordinary variable. */
  {
    (*blk)[inblk] = slot;
    slot = old_slot;
    if (++inblk > MEMSLOT_PTR_MAX)
    {
      if (MSLOT_PTR_NEXT(blk))
      {
        inblk = MEMSLOT_PTR_MIN;
        blk = MSLOT_PTR_NEXT(blk); /* "F.fwd.", we do not change bulkslot entry because we expect the effect of locality where it currently stays. */
      }
      else
        return slot ? ERR_OUT_OF_BOUND : 0; /* This is the end of the list. Any slot left means that we have not had enough room. */
    }
    old_slot = (*blk)[inblk];
  }
  return 0; /* Everything seems to be ok. */
}

/*
 * Deletes the slot under the given position.
 * (This routine is for other routines operating on memory slots.)
 */
static int kdelete_mslot(slotblk_t * blk, int inblk)
{
  /* Some checks first: */
  if (!blk)
    return ERR_BAD_PARAMETER;
  else if (inblk < MEMSLOT_PTR_MIN || inblk > MEMSLOT_PTR_MAX)
    return ERR_OUT_OF_BOUND;
  /* Initialize. */
  slotblk_t * nxtblk = (inblk < MEMSLOT_PTR_MAX) ? blk : MSLOT_PTR_NEXT(blk);
  int nxtinblk = (inblk < MEMSLOT_PTR_MAX) ? inblk : MEMSLOT_PTR_MIN;
  void * nxtslot = (nxtblk) ? (*nxtblk)[nxtinblk] : (void *)0;
  /* Loop */
  for (;;)
  {
    if (!((*blk)[inblk] = nxtslot))
      break;
    blk = nxtblk;
    inblk = nxtinblk;
    if (++nxtinblk > MEMSLOT_PTR_MAX)
    {
      nxtblk = MSLOT_PTR_NEXT(nxtblk);
      nxtinblk = MEMSLOT_PTR_MIN;
    }
    nxtslot = (nxtblk) ? (*nxtblk)[nxtinblk] : (void *)0;
  }
  /* Ok. */
  return 0;
}

/*
 * Removes a slot at the given position in the current block of a given order.
 */
static int kremove_mslot(void * slot, int order)
{
  /* Check constraints: */
  if (order < MEMSLOT_MIN_SHFT || order > MEMSLOT_MAX_SHFT)
    return ERR_OUT_OF_BOUND;
  /* Find the slot: */
  int res = kfind_mslot(slot, order, MEMSLOT_F_EXACT_MATCH);
  if (res < 0)
    return res; /* The slot was not found. */
  /* Ok, we have found it. Now, we can delete the slot from the list. */
  return kdelete_mslot(MSLOT_PTR_CURRENT(order), res);
}

/*
 * Removes slots (of any order) that starts at an address from the given range.
 */
static void kremove_mslots(void * slot_begin, void * slot_end)
{
  ptr_t b_begin = PTR_TO_PHADDR(slot_begin);
  ptr_t b_end = PTR_TO_PHADDR(slot_end);
  int inblk;
  slotblk_t * blk;
  for (int order = MEMSLOT_MIN_SHFT; order <= MEMSLOT_MAX_SHFT; order++)
  {
    if (!(blk = bulkslot[order - MEMSLOT_MIN_SHFT]))
      continue; /* The list of slots of this order is empty? */
    inblk = kfind_mslot(slot_begin, order, 0); /* No flags. */
    if (inblk < MEMSLOT_PTR_MIN || inblk > MEMSLOT_PTR_MAX)
      continue; /* Nothing to be done. */
    while ((*blk)[inblk] && BETWEEN(PTR_TO_PHADDR((*blk)[inblk]), b_begin, b_end))
    {
      kdelete_mslot(blk, inblk);
      /* Check if the last deletion deallocated current slot block: */
      if (blk != bulkslot[order - MEMSLOT_MIN_SHFT])
        break;
    }
  }
}

/*
 * Adds a slot to the appropriate list. Return 0 on success or non-zero error code
 * on failure. Some of the errors might be considered critical, and some not (e.g.
 * trying to add a slot which has already been put is not a critical error).
 */
static int kadd_mslot(void * slot, int order)
{
  /* Check constraints: */
  if (order < MEMSLOT_MIN_SHFT || order > MEMSLOT_MAX_SHFT)
    return ERR_OUT_OF_BOUND;
  /* Find the first pointer blok: */
  slotblk_t * blk = MSLOT_PTR_CURRENT(order);
  /* Check if any blok is pointed. If not, allocate a page. */
  if (!blk)
  {
    /* So... we must allocate the first page for memory slots of this order. */
    ptr_t pg = kget_free_page();
    if (!pg) /* Out of memory? */
      return ERR_OUT_OF_MEMORY; /* This is critical! */
    kclean_page(pg);
    MSLOT_PTR_CURRENT(order) = blk = PHADDR_TO_PTR(pg);
  }
  /* Find the right place to put the slot and... put it. */
  int inblk; /* Intra block index. */
  inblk = kfind_mslot(slot, order, 0); /* No flags. */
  blk = MSLOT_PTR_CURRENT(order);
  if (inblk > MEMSLOT_PTR_MAX)
  {
    /* We are in the last block and we must allocate another one. */
    ptr_t pg = kget_free_page();
    if (!pg) /* Out of memory? */
      return ERR_OUT_OF_MEMORY; /* This is critical! */
    kclean_page(pg);
    inblk = MEMSLOT_PTR_MIN; /* New block is free. */
    /* Link the new block with the list. */
    MSLOT_PTR_PREV(MSLOT_PTR_NEXT(blk) = PHADDR_TO_PTR(pg)) = blk;
    blk = MSLOT_PTR_NEXT(blk);
  }
  else if ((*blk)[inblk] == slot)
    return ERR_MEMORY_FREE; /* This slot has already been allocated (memory is already free). */
  /* We have found a place for our pointer. At the same time we must move next pointers one position forward. */
  return kinsert_mslot(slot, blk, inblk);
}

/*
 * Merge slot pointed by slot with its directly following or preceding twin (if exists).
 * This is recursive routine that, having merged some twins, tries to merge their complex
 * with a higher order twin (to form a higher order complex).
 */
static void ktry_merge_twin_mslots(void * slot, int orig_order)
{
  /* Check constraints: */
  if (orig_order > MEMSLOT_MAX_SHFT)
    return; /* We cannot merge pages to a higher size order. */
  /* Find twin and determine which one is at the lower physical address: */
  ptr_t twin_mask = (1 << orig_order); /* This mask correspond to the only bit that twins should differ. The bit depends on the size order. */
  ptr_t twin0 = PTR_TO_PHADDR(slot);
  ptr_t twin1 = twin0 ^ (twin_mask); /* Inverse the "twin" bit in order to achieve the twin physical address. */
  /* Check if the twin exists in the list: */
  int inblk = kfind_mslot(PHADDR_TO_PTR(twin1), orig_order, MEMSLOT_F_EXACT_MATCH);
  if (inblk < 0)
    return; /* The twin does not exist. */
  /* We have both twins, so we can put the lower address as a slot of a higher order. */
  ptr_t twin_lo = MIN(twin0, twin1);
  void * slot_lo = PHADDR_TO_PTR(twin_lo);
  if (orig_order < MEMSLOT_MAX_SHFT)
  {
    kadd_mslot(slot_lo, orig_order + 1);
  }
  else /* The condotion to enter here is: (orig_order == MEMSLOT_MAX_SHFT). */
  {
    kput_page(twin_lo); /* Next size "order" is actually page size, and as such memory allocation at this level is soehow different. */
  }
  kremove_mslot(PHADDR_TO_PTR(twin0), orig_order);
  kremove_mslot(PHADDR_TO_PTR(twin1), orig_order);
  /* A higher order merge trial: */
  if (orig_order < MEMSLOT_MAX_SHFT) /* Pages are not sticked together at further levels. Only slots do... */
    ktry_merge_twin_mslots(slot_lo, orig_order + 1);
}

/*
 * Marks the given memory as free. Splits the contiguous area if necessary (e.g. if
 * the parts of the area being "put" fits to different size of slots. Contrary, if
 * it is necessary, some "twins" can be joined to form a 2 times bigger slot, or even
 * a whole page. An additional parameter (order) forces given memory area to be chopped
 * off of slots of the given order (i.e. power of 2 of its size in bytes).
 * It is alsa required that the memory area is aligned with the size of the smallest
 * memory slot.
 */
static void ksplit_and_put_mslots(ptr_t b_start, ptr_t b_end, int order)
{
  /* Check if the size and the order is at least the minimum to put slot. */
  if ((b_end - b_start + 1) < MEMSLOT_MIN || order < MEMSLOT_MIN_SHFT)
    return; /* This is too small. */
  /* Find the inner area that is spanned over slots of the given order: */
  ptr_t off_mask = (1 << order) - 1;
  ptr_t b_start2 = b_start;
  ptr_t b_end2 = b_end;
  if (b_start2 & off_mask)
  {
    b_start2 |= off_mask;
    b_start2++;
    if (!b_start2)
      return; /* This was the end of physical address space... */
  }
  if ((b_end2 & off_mask) != off_mask)
  {
    b_end2 &= ~off_mask;
    if (!b_end2)
      return; /* This was the beginning of physical address space... */
    b_end2--;
  }
  /* Check if the memory area can be spanned over some slots of the given order. */
  if (b_start2 >= b_end2)
  {
    /* Try slots of lower order: */
    ksplit_and_put_mslots(b_start, b_end, order - 1);
  }
  else
  {
    /* First, call routine recursively for the remaining "edges", with a decreased order: */
    ksplit_and_put_mslots(b_start, b_start2 - 1, order - 1);
    ksplit_and_put_mslots(b_end2 + 1, b_end, order - 1);
    /* Now, put slots, trying to merge them into their "neighbours": */
    ptr_t slot_size = MSLOT_SIZE(order);
    while (b_start2 <= b_end2)
    {
      kadd_mslot(PHADDR_TO_PTR(b_start2), order);
      ktry_merge_twin_mslots(PHADDR_TO_PTR(b_start2), order); /* This find its twin (right or left, it depends) if it exists and merge them two. */
      b_start2 += slot_size;
    }
  }
}

/*
 * Recovers given memory area by putting as much memory slots as possible.
 * When recovering an area previously allocated with this same function, boundaries
 * must be extended so as to cover the extreme slots of the minimal order. The
 * previous allocation must have utilized slots to such an extends, since otherwise
 * it could not have allocated the area in question.
 * To "extend" boundaries use the flag MEMSLOT_F_INCLUSIVELY.
 */
static void kput_mslots(void * mem, int size, int flags)
{
  /* The first and the last byte of the area expressed as physical addresses: */
  ptr_t b_start, b_end;
  b_start = PTR_TO_PHADDR(mem);
  b_end = b_start + size - 1;
  /* Align them to the nearest inner address that is a multiple of the smallest slot size: */
  ptr_t b_start2, b_end2; /* Auxiliary variables. */
  b_start2 = b_start;
  b_end2 = b_end;
  if (b_start2 & MEMSLOT_MIN_OFF_MASK)
  {
    if (flags & MEMSLOT_F_INCLUSIVELY)
    {
      /* We must extend to fit the lowest order slots: */
      b_start2 &= ~MEMSLOT_MIN_OFF_MASK;
    }
    else
    {
      /* We must shrink to fit the lowest order slots: */
      b_start2 |= MEMSLOT_MIN_OFF_MASK;
      b_start2++;
      if (!b_start2)
        return; /* This was the end of physical address space... */
      size -= b_start2 - b_start; /* Shrink. */
    }
  }
  if ((b_end2 & MEMSLOT_MIN_OFF_MASK) != MEMSLOT_MIN_OFF_MASK)
  {
    if (flags & MEMSLOT_F_INCLUSIVELY)
    {
      /* We must extend to fit the lowest order slots: */
      b_end2 |= MEMSLOT_MIN_OFF_MASK;
    }
    else
    {
      /* We must shrink to fit the lowest order slots: */
      b_end2 &= ~MEMSLOT_MIN_OFF_MASK;
      if (!b_end2)
        return; /* This was the beginning of physical address space... */
      b_end2--;
      size -= b_end - b_end2; /* Shrink. */
    }
  }
  /* Check if still b_start lies before b_end: */
  if (b_start2 >= b_end2)
    return;
  /* Now, we have our memory area aligned. Call the core routine. */
  ksplit_and_put_mslots(b_start2, b_end2, MEMSLOT_MAX_SHFT); /* Try to alocate the biggest slots first. */
}

/*
 * This is the "false bitmap" described in details in "bulkpg" table description:
 */
static u8 fls_pg_bmap[PG_BITMAP_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }; /* 128 elements x 8 bits = 1024 bits */

/*
 * Bitmap for the first 4Mb. The first 1MB of RAM is "excluded" by default (0x00000000 - 0x000fffff,
 * pages from 0 to 255). The pointer to page #0 is reserved as "NULL" and we have the page #0
 * marked occupied. We also expect the main C code (booting code) to be located somewhere between 31KB
 * and 639 KB (0x00007c00 - 0x009fbff). The kernel must manage this memory itself if it wants to.
 * Another issue we workaround the above way is allocation memory for free-memory bitmaps (those 
 * 128B long). We keep pointers to those structers as C code pointers (shifted by the C code beginning
 * physical address), so these structures cannot fall into the memory below the limit that C code can
 * handle.
 * Nevertheless, this memory can be used for something usefull (e.g. for IDT, GDT, LDT, etc...).
 * Further exclusions will be done by the initialization routine. See: "bulkpg" description.
 */
static u8 fst_pg_bmap[PG_BITMAP_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/*
 * 1024 pointers to the 128B-long structures - free pages bitmap.
 * Each of the possible 1024 strucutures describe its own 1024 pages.
 * (1024 structures x 1024 pages x 4kB = all the 32-bit memory).
 *
 * First pointer links us to a bitmap describing first 1024 pages
 * (which cover the first 4MB). Second pointer - physical memory
 * between 4 and 8 MB, and so on... Null pointer means, that all
 * of the 1024 related to this pointer are free, but allocating
 * any page from this range will involve creating 128B-long bitmap
 * first (128B x 8bits = 1024, hence the number of pages covered).
 *
 * It is possible that two pointer point to the same bitmap. This
 * is used to cover unreachable memory via a type of "false" bitmap
 * with each bit set to 1, which means that all of the pages described
 * by such a bit has been already "allocated" (i.e. "not allocatable").
 */
static u8 * bulkpg[PG_BITMAPS] = { fst_pg_bmap, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, fls_pg_bmap, fls_pg_bmap, fls_pg_bmap, fls_pg_bmap, fls_pg_bmap };

u8 * hwm_bmap = 0x00000000; /* If not 0, this is the first free space to allocate next bitmap. */

/*
 * Finds free page. Returns its physical addres, or 0 if not allocated.
 */
ptr_t kget_free_page()
{
  /* Declare: */
  ptr_t pg = 0;
  int bmapno = 0;
  /*
   * Let's iterate over all the bitmaps (or until free page is allocated) for the
   * first time and try to find an area already "bitmapped".
   */
  while (bmapno < PG_BITMAPS && pg == 0)
  {
    if (bulkpg[bmapno] != 0)
    {
      u8 * bmap = bulkpg[bmapno];
      int qnum = 0; /* qnum is the index of a byte in the bitmap (covering 8 x 4096 = 32kB). */
      while (qnum < PG_BITMAP_SIZE && pg == 0)
      {
        if (bmap[qnum] != 0xff) /* If not all bits are 1, then at least one is 0 - it means that the corresponding page is free. */
        {
          u8 mask = 1; /* Rotating mask to do some bit test. */
          pg = bmapno * 0x00400000; /* Each bitmap describes 4Mb (0x400000) */
          pg += qnum * 0x8000; /* Each byte in the bitmap describes 32Kb (0x8000). */
          while (mask & bmap[qnum]) /* It must stop, since some bit is 0 (not all are 1). */
          {
            mask <<= 1;
            pg += 0x1000; /* Each bit describes 1 page - 4Kb (0x1000). */
          }
          bmap[qnum] |= mask; /* Set the appropriate bit to mark the corresponding page occupied. */
        }
        qnum++;
      }
    }
    bmapno++;
  }
  /*
   * If we have not found free page yet, we must iterate over bitmap pointers table
   * again, find a free page, create bitmap, and so on...
   */
  bmapno = 0; /* "Rewind" */
  while (bmapno < PG_BITMAPS && pg == 0)
  {
    if (bulkpg[bmapno] == 0)
    {
      int i; /* Iterator */
      u8 * bmapel; /* Bitmap element (for iteration). */
      pg = bmapno << PG_BMAPNO_SHFT; /* It may be increased another 4Kb (page size) forward, if the page is needed to allocate next bitmap. */
      if (!hwm_bmap)
      {
        /* Allocate memory for a bitmap first. This is going to be the first page of the area covered by this element of bulkpg. */
        bulkpg[bmapno] = (u8 *)(pg - ph_addr); /* Remember to subtract physical offset so as to obtain C code compliant (relative) pointer .*/
        /* Set bitmap HWM just after the above (128 byte long) bitmap. */
        hwm_bmap = bulkpg[bmapno] + PG_BITMAP_SIZE;
        /* Set underlying page allocated, as well as the next one. The first one - allocated for the bitmap, the second one - requested by the function caller. */
        *(bmapel = bulkpg[bmapno]) = 0x03;
        /* Increase page address due to the bitmap allocation just made. */
        pg += PG_SIZE;
      }
      else
      {
        /* We have already had a free area to locate new bitmap at. */
        bulkpg[bmapno] = hwm_bmap;
        /* Move HWM 128 bytes above the current addres. */
        hwm_bmap += PG_BITMAP_SIZE;
        /* Check if this still points to the same page. If not, clear HWM, so that the next allocation set it itself. */
        if (!(((ptr_t)hwm_bmap + ph_addr) & PG_OFFSET)) /* Remember, to check page-align on physical-address basis (not C-code-relative basis). */
          hwm_bmap = 0;
        /* Set the first bit of the bitmap, pad with 0 until the end of the first byte. The bit set to 1 corresponds to the page allocated for caller. */
        *(bmapel = bulkpg[bmapno]) = 0x01;
      }
      /* Clear the bitmap, except the first byte, which has already been initialized: */
      i = 1;
      while (i < PG_BITMAP_SIZE)
      {
        *(++bmapel) = 0x00;
        i++;
      }
      /* Return the page found (we can stop iterating): */
      return pg;
    }
    bmapno++;
  }
  
  return pg;
}

/*
 * Checks whether the physical page at the provided address is free. Returns non-0 if so.
 */
int ktest_page_free(ptr_t pg_addr)
{
  /* Check which series (bitmap) this page belongs to: */
  int bmapno = (int)((pg_addr & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT);
  if (!bulkpg[bmapno])
    return 1; /* This page belongs to the area that do not even have its bitmap initialized. So, it is free for sure. */
  /* Check which byte and bit of the appopriate bitmap this page belongs to: */
  int qnum = (int)((pg_addr & PG_BYTEIDX_MASK) >> PG_BYTEIDX_SHFT);
  int mask = 0x01 << (int)((pg_addr & PG_BITIDX_MASK) >> PG_BITIDX_SHFT);
  /* Test and return the result. */
  return !(mask & (int)*(bulkpg[bmapno] + qnum));
}

/*
 * Sets the corresponding bit to mark the page allocated.
 * Be carefull, it does nothing if the page belongs to the
 * area which has not yet been bitmapped!
 */
static void kset_page_allocated(ptr_t pg_addr)
{
  /* Check which series (bitmap) this page belongs to: */
  int bmapno = (int)((pg_addr & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT);
  if (!bulkpg[bmapno])
    return; /* This page belongs to the area that do not have its bitmap initialized. */
  /* Check which byte and bit of the appopriate bitmap this page belongs to: */
  int qnum = (int)((pg_addr & PG_BYTEIDX_MASK) >> PG_BYTEIDX_SHFT);
  int mask = 0x01 << (int)((pg_addr & PG_BITIDX_MASK) >> PG_BITIDX_SHFT);
  /* Set the corresponding bit: */
  *(bulkpg[bmapno] + qnum) |= (u8)mask;
}

/*
 * Checks whether the page belong to an area already bitmapped.
 */
static int ktest_page_bitmapped(ptr_t pg_addr)
{
  int bmapno = (int)((pg_addr & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT);
  return (bulkpg[bmapno] != 0);
}

/*
 * Finds a number of free consistent pages. Returns the physical address of the first one,
 * or 0, if the allocation failed. If 0 is passed, the allocator will try to reserve all
 * the memory - this will fail, of course.
 */
ptr_t kget_free_pages(u32 pages)
{
  /* Decl. */
  ptr_t pg = 0; /* Page physical address. */
  ptr_t pg0; /* The page beginning the currently checked consistent area. */
  u32 cnst = 0; /* The number of free consistent pages already found. */
  u32 fbmpent; /* Free bitmap entries in a page already allocated for this purpose. */
  u32 addpg; /* Additional pages required for bitmaps in the currently scanned consistent area. */
  int last0bmp; /* Index of the last unallocated bitmap found (0..1023). Initialized with 0, although bitmap #0 is always set up at system startup. */
  int currbmp; /* Index of the current bitmap corresponding to the currently scanned page.. */
  /* Iterate through the pages: */
  do
  {
    if (ktest_page_free(pg))
    {
      if (cnst == 0)
      {
        pg0 = pg; /* Consistent area starts here. */
        addpg = 0; /* Again, we do not need any additional pages yet. */
        fbmpent = hwm_bmap ? ((PG_SIZE - (((u32)hwm_bmap + ph_addr) & PG_OFFSET)) / PG_BITMAP_SIZE) : 0; /* Perhaps, we have some free bitmap entries. Be carefull: hwm_bmap is expressed as a relative-to-C address! */
        last0bmp = -1; /* Clear the "last virtually bitmapped" mark. We must set below 0, since we are not sure whether the first 4MB area (bitmap #0) has already have its own bitmap. */
      }
      if (!ktest_page_bitmapped(pg))
      {
        currbmp = (int)((pg & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT);
        if (currbmp != last0bmp)
        {
          /* We have not count yet the number of additional pages necessary for this area. */
          if (fbmpent)
            fbmpent--; /* We had some free left in the current "bitmaps page". */
          else
          {
            addpg++; /* It seems that now another bitmap will need a new page. */
            fbmpent = PG_SIZE / PG_BITMAP_SIZE - 1; /* New page, of course, will have some more free entries. */
          }
          last0bmp = currbmp; /* Mark this 4MB area as "already virtually" bitmapped. */
        }
      }
      if (++cnst == pages + addpg) /* We must also allocate additional pages (for bitmaps), if necessary. */
      {
        /* First free page for bitmap allocation of the newly allocated pages. */
        ptr_t pgbmp = pg0;
        /* Reinitiate the number of free bitmap entries: */
        fbmpent = hwm_bmap ? ((PG_SIZE - (((u32)hwm_bmap + ph_addr) & PG_OFFSET)) / PG_BITMAP_SIZE) : 0; /* Convert hwm_bmap to physical address. */
        /* Mark appropriate pages allocated. Allocate bitmaps, if necessary. */
        pg = pg0; /* "Rewind". */
        pages += addpg; /* We do not need "pages" parameter anymore. It is the best choice to countdown. */
        /* Iterate over all of the pages (inlcuding those, allocated for bitmaps): */
        while (pages--)
        {
          if (!ktest_page_bitmapped(pg))
          {
            /* So... we have to allocate the appropriate bitmap first, before marking this page allocated. */
            if (fbmpent)
            {
              /* At least one free bitmap entry exists in the "current bitmaps page". */
              bulkpg[(pg & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT] = hwm_bmap; /* Point to the newly allocated bitmap. */
              for (int i = 0; i < PG_BITMAP_SIZE; i++)
                hwm_bmap[i] = 0x00; /* Clear the bitmap. */
              hwm_bmap += PG_BITMAP_SIZE; /* Next bitmap entry just above this one (if there is enough room)... */
              fbmpent--; /* We have just utilized one entry. */
            }
            else
            {
              fbmpent = PG_SIZE / PG_BITMAP_SIZE - 1; /* All the entries from the newly allocated page, except one to be allocated in a moment. */
              hwm_bmap = (u8 *)(pgbmp - ph_addr); /* Remember to compute relative-to-C address from the physical one. */
              bulkpg[(pg & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT] = hwm_bmap; /* Point to the newly allocated bitmap. */
              for (int i = 0; i < PG_BITMAP_SIZE; i++)
                hwm_bmap[i] = 0x00; /* Clear the bitmap. (The underlying page is going to be merked as used in a moment.) */
              pgbmp += PG_SIZE; /* Potentially next bitmap containing page will be allocated just after this one (but still below our "contiguous" - i.e. allocated - area starts). */
              hwm_bmap += PG_BITMAP_SIZE; /* Next bitmap entry just above this one... */
            }
          }
          /* It is asserted that the appropriate bitmap exists. Now, mark the page occupied. */
          kset_page_allocated(pg);
          /* Next page: */
          pg += PG_SIZE;
        }
        /* Return pointer: */
        return pg0 + PG_SIZE * addpg; /* We must shift returned address, because some pages might have just been allocated for bitmaps. */
      }
    }
    else
      cnst = 0; /* Consistent area has already ended or it has not even been found yet. */
    pg += PG_SIZE; /* Go on to the next physical page. */
  } while (pg != 0); /* So as not to spin forever if all the memory is occupied. */
  return 0;
}

/*
 * Headers to move back and forth through the memory slots of any order.
 */
typedef struct
{
  slotblk_t * blk; /* Current block. */
  int inblk;       /* Intra block pointer. */
  int order;       /* Block list order. */
} mslot_hdr_t;

static mslot_hdr_t mslot_hdrs[MEMSLOT_SHFT_RANGE]; /* From min to max order. */

static void mslot_hdr_init(mslot_hdr_t * mslot_hdr, int order)
{
  mslot_hdr->blk = bulkslot[order - MEMSLOT_MIN_SHFT];
  if (mslot_hdr->blk)
    while (MSLOT_PTR_PREV(mslot_hdr->blk))
      mslot_hdr->blk = MSLOT_PTR_PREV(mslot_hdr->blk);
  mslot_hdr->inblk = MEMSLOT_PTR_MIN;
  mslot_hdr->order = order;
}

static void mslot_hdrs_init(void)
{
  for (int i = 0; i < MEMSLOT_SHFT_RANGE; i++)
    mslot_hdr_init(&mslot_hdrs[i], i + MEMSLOT_MIN_SHFT);
}

/*
 * Checks if the next pointer from the list of slots of some order can be read.
 */
static int mslot_hdr_has_next(mslot_hdr_t * mslot_hdr)
{
  /* Some checks: */
  if (!mslot_hdr)
    return 0;
  /* Finally... */
  int ok = (mslot_hdr->blk) ? (mslot_hdr->inblk <= MEMSLOT_PTR_MAX) : 0;
  return ok ? ((*(mslot_hdr->blk))[mslot_hdr->inblk] != (void *)0) : 0;
}

/*
 * This function is returns slot at the current position and does not
 * move to the next slot afterward. This means that any consecutive
 * call to this funciton will return the same result each time.
 */
static void * mslot_hdr_peek_next(mslot_hdr_t * mslot_hdr)
{
  return (mslot_hdr_has_next(mslot_hdr)) ? ((*(mslot_hdr->blk))[mslot_hdr->inblk]) : (void *)0;
}

/*
 * Moves to the the next pointer from the list of slots of some order.
 * Using this along with mslot_hdr_peek_next() allows to iterate over
 * all the slots.
 */
static void mslot_hdr_next(mslot_hdr_t * mslot_hdr)
{
  if (!mslot_hdr)
    return; /* Check */
  mslot_hdr->inblk++;
  if (mslot_hdr->inblk > MEMSLOT_PTR_MAX && MSLOT_PTR_NEXT(mslot_hdr->blk))
  {
    mslot_hdr->blk = MSLOT_PTR_NEXT(mslot_hdr->blk);
    mslot_hdr->inblk = MEMSLOT_PTR_MIN;
  }
}

/*
 * Check if any head (of any order) "has next" pointer to get.
 */
static int mslot_hdrs_has_next(void)
{
  /* Loop over all the size orders and check out if any slot of such order "has next": */
  for (int i = 0; i < MEMSLOT_SHFT_RANGE; i++)
  {
    if (mslot_hdr_has_next(&mslot_hdrs[i]))
      return 1; /* So... we have found one! */
  }
  /* We did not find any "having next". */
  return 0;
}

typedef struct
{
  void * addr; /* Memory slot address. */
  int order;  /* Memory slot size order. */
} mslot_info_t;

/*
 * This function returns the next lowest (in terms of its address) memory slot
 * via appropriate head.
 */
static mslot_info_t mslot_hdrs_get_next(void)
{
  /* Declare and initialze return value (info structure). */
  mslot_info_t mslot_info;
  mslot_info.addr = (void *)0;
  mslot_info.order = -1;
  /* Loop over all the size orders and find: */
  for (int i = 0; i < MEMSLOT_SHFT_RANGE; i++)
  {
    if (!mslot_info.addr)
    {
      if (mslot_hdr_has_next(&mslot_hdrs[i]))
      {
        mslot_info.addr = mslot_hdr_peek_next(&mslot_hdrs[i]); /* Get next pointer from the list of corresponding order. */
        mslot_info.order = i + MEMSLOT_MIN_SHFT; /* Orders are in range MEMSLOT_MIN_SHFT..MEMSLOT_MAX_SHFT, but indexes are "shifted" to start from 0. */
      }
    }
    else if (mslot_hdr_has_next(&mslot_hdrs[i]))
    {
      void * cdtaddr = mslot_hdr_peek_next(&mslot_hdrs[i]); /* Current "Candidate" pointer. */
      if (PTR_TO_PHADDR(cdtaddr) < PTR_TO_PHADDR(mslot_info.addr))
      {
        /* Current candidate's address is lower than the previous candidate: */
        mslot_info.addr = cdtaddr; /* Get next pointer from the list of corresponding order. */
        mslot_info.order = i + MEMSLOT_MIN_SHFT; /* Orders are in range MEMSLOT_MIN_SHFT..MEMSLOT_MAX_SHFT, but indexes are "shifted" to start from 0. */
      }
    }
  }
  /* Although we have already got the address, we must do a dummy "get next" in order to move a corresponding head to the next position: */
  if (mslot_info.addr && mslot_info.order != -1)
    mslot_hdr_next(&mslot_hdrs[mslot_info.order - MEMSLOT_MIN_SHFT]); /* This assigns nothing new... */
  /* Return what we have found: */
  return mslot_info;
}

/*
 * Puts "memory size" variable at the address just before (!!!) where p points.
 * The main purpose of this function is to put area size associated with a pointer to it.
 * This is stored in the "big endian" manner.
 */
static void kput_msz(void * p, msz_t msz)
{
  int i = sizeof(msz_t);
  char * pc = (char *)p;
  while (i--)
  {
    pc--;
    *pc = (char)(msz & 0xff);
    msz >>= 8;
  }
}

/*
 * Gets "memory size" variable from the address just before (!!!) where p points.
 * The main purpose of this function is to get area size associated with a pointer to it.
 * The stored value must be "big endian".
 */
static msz_t kget_msz(void * p)
{
  int i = 0;
  msz_t msz = 0;
  char * pc = (char *)p;
  do
  {
    pc--;
    msz += (*pc) << (8 * i);
  } while (i++ < sizeof(msz_t));
  return msz;
}

/*
 * Allocates a number of adjacent memory slots so as to gain a memory area
 * of the given size. A pointer returned is aligned as requested.
 */
static void * kallocate_mslots(msz_t size, u16 algn)
{
  /* Some check: */
  if (size <= 0 || size >= (1 << (MEMSLOT_MAX_SHFT + 1)) || algn > MEMSLOT_MAX_ALIGN)
    return (void *)0; /* Returning null pointer means that we could not allocate or an error occured. */
  if (!algn)
    algn = 1; /* This is the default. */
  /* Initialize. "Rewind" the collection of memory slots: */
  void * ptr = (void *)0; /* Value returned. */
  void * ptr0 = (void *)0; /* Auxiliary pointer (first slot of adjacent ones or null). */
  void * ptre = (void *)0; /* Auxiliary pointer (next pointer expected if adjacent). */
  ptr_t size0; /* Overall size discovered of current adjacent slots. */
  ptr_t algn0; /* Current adjacent slots area size lost due to alignemt requirement. */
  mslot_info_t mslot_info;
  mslot_hdrs_init();
  /* Look for adjacent slots that will give enough room. */
  while (!ptr && mslot_hdrs_has_next())
  {
    /* Get next slot:*/
    mslot_info = mslot_hdrs_get_next();
    /* Stick slots together: */
    if (!ptr0 || PTR_TO_PHADDR(mslot_info.addr) != PTR_TO_PHADDR(ptre))
    {
      /* This is the first slot of adjacent slots area: */
      algn0 = sizeof(msz_t) + (algn - (sizeof(msz_t) + (ptr_t)(mslot_info.addr)) % algn) % algn; /* This is the number of bytes to loose due to the alignment (this also includes the place for variable holding the area size). */
      size0 = 1 << mslot_info.order;
      if (algn0 < size0 + sizeof(msz_t)) /* Alignment cannot exceed the size of this slot by more than the size of the variable holding area size (4 bytes in 32-bit architecures). Otherwise there would be no point in occupying the slot. */
      {
        ptr0 = mslot_info.addr;
        ptre = PHADDR_TO_PTR(PTR_TO_PHADDR(ptr0) + (size0)); /* We expect next pointer to point just behind the current slot. */
      }
    }
    else if (PTR_TO_PHADDR(mslot_info.addr) == PTR_TO_PHADDR(ptre)) /* The condition here is always true if the code execution reaches it. */
    {
      /* Current slot is adjacent to the previously found one. */
      size0 += 1 << mslot_info.order;
      ptre = PHADDR_TO_PTR(PTR_TO_PHADDR(ptre) + (1 << mslot_info.order));
    }
    /* Check if adjacent slots area is enough: */
    if (size0 >= size + algn0)
    {
      ptr = PHADDR_TO_PTR(PTR_TO_PHADDR(ptr0) + algn0); /* We have found it! */
      /* Remove slots from the list: */
      kremove_mslots(ptr0, PTR_ADD_BYTES(ptr0, size0 - 1));
      /* Recover relict area: */
      if (algn0 - sizeof(msz_t))
        kput_mslots(ptr0, (int)algn0 - sizeof(msz_t), 0);
      if (PTR_TO_PHADDR(ptr) + size <= PTR_TO_PHADDR(ptre) - 1)
        kput_mslots(PHADDR_TO_PTR(PTR_TO_PHADDR(ptr) + size), PTR_TO_PHADDR(ptre) - PTR_TO_PHADDR(ptr) - size, 0);
    }
  }
  /* Store memory area size (memory for this variable is already allocated). */
  kput_msz(ptr, size);
  /* Finish. */
  return ptr;
}

#define KMALLOC_COUNT_PAGES(size) ((size) / PG_SIZE + ((size) % PG_SIZE != 0))

/*
 * Allocates memory, not less than requested, and aligned - as requested.
 * (Alignment relates to C-code-relative, not physical address!)
 * Parameters are measured in bytes. Functions return 0 if it cannot allocate
 * memory. It is also important that the function is told the real offset between
 * physical memory (which starts at physical address 0x00000000, of course) and
 * the beginning of the C code (which starts somewhere above 0x00007c00).
 *
 * Returning pointer refers to the memory as it is seen by the C code: starting at
 * 0x00000000 although physically it strats above 0x00007c00. For instance, if
 * the C code starts at 0x00008200, a pointer to the physical memory at 0x00008400
 * is 0x00000200 (since this is how C code sees it here).
 *
 * Finally, this is important, that the memory can be allocated in kernel mode only.
 * So, any kernel code can allocate and free any fragment of it.
 */
void * kmalloc(msz_t size, u16 algn)
{
  /* Some check: */
  if (!size)
    return (void *)0;
  else if (algn > MEMSLOT_MAX_ALIGN) /* Page size is the limit of alignment. */
    return (void *)0;
  if (!algn)
    algn = 1; /* The default alignment is "no alignment". */
  /* Number of contiguous pages: */
  u32 pages = KMALLOC_COUNT_PAGES(size + algn + sizeof(msz_t) - 1);
  if (pages > 1)
  {
    /* Get the appropriate number of pages: */
    ptr_t phalloc = kget_free_pages(pages);
    /* Check whether the allocation has succeeded: */
    if (!phalloc)
      return (void *)0; /* Failure. */
    /* Count the alignment loss (including the area for "memory size" variable): */
    msz_t algn0 = sizeof(msz_t) + (((msz_t)algn - (sizeof(msz_t) + PHADDR_TO_CADDR(phalloc)) % algn) % algn);
    /* Try to recover relict areas: */
    if (algn0 - sizeof(msz_t))
      kput_mslots(PHADDR_TO_PTR(phalloc), (int)algn0 - sizeof(msz_t), 0); /* Leave an area for "memory size" field. */
    ptr_t la = phalloc + algn0 + size - 1; /* The last allocated byte. */
    ptr_t lp = phalloc + pages * PG_SIZE - 1; /* The last byte of the last page allocated. */
    if (la < lp)
      kput_mslots(PHADDR_TO_PTR(la + 1), (int)(lp - la), 0);
    /* Store the "memory area size" field: */
    void * ret_p = PHADDR_TO_PTR(phalloc + algn0);
    kput_msz(ret_p, size);
    /* Return pointer. */
    return ret_p; /* We must remember to convert physical address to C-relative-pointer. */
  }
  else if (pages == 1)
  {
    /* Try to find appropriate slot(s) first. */
    void * slot = kallocate_mslots(size, algn);
    if (slot)
      return slot; /* Memory slots allocation has succeeded... */
    /* We must allocate a page */
    ptr_t phalloc = kget_free_page();
    if (!phalloc)
      return (void *)0; /* Failure. */
    /* Count the alignment loss: */
    msz_t algn0 = sizeof(msz_t) + (((ptr_t)algn - (sizeof(msz_t) + PHADDR_TO_CADDR(phalloc)) % algn) % algn);
    /* Try to recover relict areas: */
    if (algn0)
      kput_mslots(PHADDR_TO_PTR(phalloc), (int)algn0 - sizeof(msz_t), 0); /* Leave an area for "memory size" field. */
    ptr_t la = phalloc + algn0 + size - 1; /* The last allocated byte. */
    ptr_t lp = phalloc + PG_SIZE - 1; /* The last byte of the last page allocated. */
    if (la < lp)
      kput_mslots(PHADDR_TO_PTR(la + 1), (int)(lp - la), 0);
    /* Store the "memory area size" field: */
    void * ret_p = PHADDR_TO_PTR(phalloc + algn0);
    kput_msz(ret_p, size);
    /* Return pointer. */
    return ret_p; /* We must remember to convert physical address to C-relative-pointer. */
  }
  else
    return (void *)0;
}

/*
 * Returns a page back to the kernel memory pool. The page is pointed by the physical address
 * of its beginning. The function does not check if actually this page has been free, and marks
 * it free anyway.
 */
void kput_page(ptr_t pg_addr)
{
  if (!ktest_page_free(pg_addr)) /* This also checks if the page has bitmap. */
  {
    /* Check which series (bitmap) this page belongs to: */
    int bmapno = (int)((pg_addr & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT);
    if (!bulkpg[bmapno])
      return; /* This page belongs to the area that do not even have its bitmap initialized. So, it is free for sure. */
    /* Check which byte and bit of the appopriate bitmap this page belongs to: */
    int qnum = (int)((pg_addr & PG_BYTEIDX_MASK) >> PG_BYTEIDX_SHFT);
    int mask = 0x01 << (int)((pg_addr & PG_BITIDX_MASK) >> PG_BITIDX_SHFT);
    /* Clear the bit: */
    *(bulkpg[bmapno] + qnum) &= ~mask;
  }
}

/*
 * Similar to kput_page() but frees a number of contiguous pages.
 */
void kput_pages(ptr_t pg, int pages)
{
  while (pages--)
  {
    kput_page(pg);
    pg += PG_SIZE;
  }
}

/*
 * Frees the given amount of memory at the given address.
 */
void kfree(void * mem)
{
  /* Check for null pointer: */
  if (!mem)
    return; /* This is permissible to "free" null pointer. By default this does nothing (it even does not raise any error). */
  /* Get the memory area size: */
  msz_t size = kget_msz(mem);
  kput_mslots(PTR_ADD_BYTES(mem, -sizeof(msz_t)), size + sizeof(msz_t), MEMSLOT_F_INCLUSIVELY); /* There must have been allocated small area for the "size" field. */
}

/*
 * Cleans the given page.
 */
void kclean_page(ptr_t pg)
{
  static u16 kernel_big_data = GDT_KERNEL_BIG_DATA;
  __asm__ __volatile__ ("\n\t"
    "pushf \n\t"
    "push %%es \n\t"
    "mov %%ax,%%es \n\t"
    "xor %%eax,%%eax \n\t"
    "cld \n\t"
    "mov $1024,%%ecx \n\t"
    "rep \n\t"
    "stosl \n\t"
    "pop %%es \n\t"
    "popf \n\t"
    :
    : "a" (kernel_big_data), "D" (pg)
    : "%ecx"
  );
}

/*
 * Look for the address range descriptor of the lowest base among those
 * with base not below the limit given.
 */
static addr_range_desc_t * find_rdesc_lw(ptr_t lower_limit)
{
  addr_range_desc_t * rdesc_p = (addr_range_desc_t *)0;
  mmap_t mmap = get_memory_map();
  for (int i = 0; i < mmap.length; i++)
    if (mmap.rdesc[i].base_address0 >= lower_limit)
    {
      if (rdesc_p)
      {
        if (mmap.rdesc[i].base_address0 < rdesc_p->base_address0)
          rdesc_p = &mmap.rdesc[i];
      }
      else
        rdesc_p = &mmap.rdesc[i];
    }
  return rdesc_p;
}

/*
 * Creates a bitmap for the physical memory area (of 4MiB length).
 * A newly created bitmap should reside at the address that does not exceed
 * the imposed limits. This does not apply to a bitmap put in a block of
 * currently allocated pages - this one is considered to belong to an already
 * initialized memory area.
 * If a bitmap already exists, the routine does nothing.
 * This routine is designed for use in the boot process only.
 * Returns 0 on succes, error code on failure.
 */
static int kbitmap_page(ptr_t addr, ptr_t lo_lmt, ptr_t hi_lmt)
{
  /* Check if a bitmap exists: */
  int bmapno = PGADDR_TO_BMAPNO(addr);
  if (bulkpg[bmapno]) /* We do not utilize ktest_page_bitmapped() since we must count bitmap index anyway. */
    return 0; /* Bitmap exists... We consider it success. */
  /* Check if there is some free memory in the block of currently created bitmaps: */
  if (!hwm_bmap)
  {
    /* Look for a page within the limit that is free and already bitmapped. */
    for (ptr_t pg = lo_lmt; pg <= hi_lmt; pg += PG_SIZE)
    {
      if (ktest_page_free(pg) && ktest_page_bitmapped(pg))
      {
        /* Set page allocated - it will contain the requested bitmap. */
        hwm_bmap = PHADDR_TO_PTR(pg);
        kset_page_allocated(pg);
        break;
      }
    }
    /* If still not found...: */
    if (!hwm_bmap)
    {
      /* Look for a page within the limit that is free. It will be bitmapped.  */
      for (ptr_t pg = lo_lmt; pg <= hi_lmt; pg += PG_SIZE)
      {
        if (ktest_page_free(pg) && !ktest_page_bitmapped(pg))
        {
          /* First, the bitmap for the "bitmap" page (the page will contain its bitmap itself). */
          hwm_bmap = PHADDR_TO_PTR(pg);
          int bmapno2;
          for (int i = 0; i < PG_BITMAP_SIZE; i++)
            hwm_bmap[i] = 0x00;
          bulkpg[bmapno2 = PGADDR_TO_BMAPNO(pg)] = hwm_bmap;
          hwm_bmap += PG_BITMAP_SIZE;
          /* Ok, now. Set this page allocated. It will contain both the request and its own bitmaps. */
          kset_page_allocated(pg);
          /* Check if, by a "strange coincidence", the requested bitmap is the same as the above: */
          if (bmapno == bmapno2)
          {
            /* Set the requested page allocated. */
            kset_page_allocated(addr);
            /* Move HWM, and check if it overflows the current page. */
            hwm_bmap += PG_BITMAP_SIZE;
            if (!(PTR_TO_PHADDR(hwm_bmap) & PG_OFFSET))
              hwm_bmap = 0; /* Seems as if it overflowed... */
            /* Done. */
            return 0;
          }
          /* Go to the core routine: */
          break;
        }
      }
      if (!hwm_bmap)
        return ERR_MEMORY_ALLOCATION; /* Ooops! */
    }
  }
  /* Initialize bitmap: */
  for (int i = 0; i < PG_BITMAP_SIZE; i++)
    hwm_bmap[i] = 0x00;
  /* Assign the bitmap: */
  bulkpg[bmapno] = hwm_bmap;
  /* Set the requested page allocated. */
  kset_page_allocated(addr);
  /* Move HWM, and check if it overflows the current page. */
  hwm_bmap += PG_BITMAP_SIZE;
  if (!(PTR_TO_PHADDR(hwm_bmap) & PG_OFFSET))
    hwm_bmap = 0; /* Seems as if it overflowed... */
  /* Everything seems to be ok: */
  return 0;
}

/*
 * This is similar to kbitmap_page(), but it imposes lower address limit only
 * and utilzes the memory map to find actually free memory regions. Of course,
 * only regions above the limit will respected.
 * In fact, this function iteratively calls kbitmap_page(). Iterations are
 * performed until kbitmap_page() succeeds on the range "taken" as free from
 * memory map. If no iteration succeeds, the whole function succeeds neither.
 */
static int kbitmap_page2(ptr_t addr, ptr_t lo_lmt, mmap_t mmap)
{
  addr_range_desc_t * rdesc;
  ptr_t lo, hi;
  /* Iterate over the memory map and find a free range. */
  for (int i = 0; i < mmap.length; i++)
  {
    rdesc = &(mmap.rdesc)[i];
    if (rdesc->type != AD_RNG_TYP_AVAILABLE) /* "ACPI reclaim" type is not treated as free for writing at this stage of the booting procedure. */
      continue;
    lo = MAX(lo_lmt, rdesc->base_address0);
    hi = rdesc->base_address0 + rdesc->length0 - 1;
    if (lo <= hi)
      if (!kbitmap_page(addr, lo, hi))
        return 0;
  }
  /* No iteration succeeded: */
  return ERR_MEMORY_ALLOCATION;
}

/*
 * Initialization routine.
 */
void init_mem_mgr(void)
{
  /*
   * Mark each full-4MiB area above the RAM limit as "occupied".
   */
  ptr_t noram = get_mem_limit() + 1;
  /* Move to the nearest 4MiB boundary: */
  if (noram & ~PG_BMAPNO_MASK)
    noram = (noram | ~PG_BMAPNO_MASK) + 1;
  /* Iterate over all 4MiB areas above the RAM limit */
  msz_t asize = 1 << PG_BMAPNO_SHFT; /* 4MiB (covered by 1 bitmap) */
  int bmapno; /* Bitmap no of the area. */
  while (noram)
  {
    bmapno = (int)((noram & PG_BMAPNO_MASK) >> PG_BMAPNO_SHFT);
    bulkpg[bmapno] = fls_pg_bmap; /* "False" bitmap to mark an area totally unusable. */
    noram += asize;
  }
  /*
   * Now, go on to the memory belowthe RAM limit.
   */
  ptr_t lw_lmt = 0x00000000;
  addr_range_desc_t * rdesc_p = find_rdesc_lw(lw_lmt);
  ptr_t lo, hi; /* Auxiliary boundaries. */
  ptr_t up_lmt = get_mem_limit() + 1; /* 4MiB - RAM limit ceiled at the nearest 4MiB boundary. */
  if (up_lmt & ~PG_BMAPNO_MASK)
    up_lmt = (up_lmt | ~PG_BMAPNO_MASK) + 1;
  up_lmt--;
  while (rdesc_p)
  {
    /* Check limit: */
    if (rdesc_p->base_address0 + rdesc_p->length0 - 1 < 0x00100000)
    {
      lw_lmt = rdesc_p->base_address0 + rdesc_p->length0;
      rdesc_p = find_rdesc_lw(lw_lmt);
      continue; /* Everything below the first 1MiB has already been marked as used. */
    }
    else if (rdesc_p->base_address0 > up_lmt)
      break; /* This is above the limit... */
    /* Check if there is a hole in the map: */
    if (rdesc_p->base_address0 > lw_lmt)
    {
      /* Indeed... Mark the hole as used. */
      lo = MAX(0x00100000, lw_lmt);
      hi = MIN(up_lmt, rdesc_p->base_address0  - 1);
      if (lo <= hi)
        for (ptr_t pg = lo & ~PG_OFFSET; pg <= hi; pg += PG_SIZE)
        {
          if (!ktest_page_bitmapped(pg))
          {
            if (kbitmap_page2(pg, 0x00100000, get_memory_map()))
            {
              tprintfk("Cannot initialize memory allocation structures for the range 0x%8x-0x%8x.\n", lo, hi);
              kpanic(); /* This is critical error! */
            }
          }
          kset_page_allocated(pg); /* This callee needs the page being already bitmapped. Well... it is, in fact. */
        }
    }
    /* Now, check the address range descriptor: */
    if (rdesc_p->type != AD_RNG_TYP_AVAILABLE && rdesc_p->type != AD_RNG_TYP_ACPI_RECL) /* Type 1 and 3 are considered as a free memory area. */
    {
      lo = MAX(0x00100000, rdesc_p->base_address0);
      hi = MIN(up_lmt, rdesc_p->base_address0 + rdesc_p->length0 - 1);
      if (lo <= hi)
        for (ptr_t pg = lo & ~PG_OFFSET; pg <= hi; pg += PG_SIZE)
        {
          if (!ktest_page_bitmapped(pg))
          {
            if (kbitmap_page2(pg, 0x00100000, get_memory_map()))
            {
              tprintfk("Cannot initialize memory allocation structures for the range 0x%8x-0x%8x.\n", lo, hi);
              kpanic(); /* This is critical error! */
            }
          }
          kset_page_allocated(pg); /* This callee needs the page being already bitmapped. Well... it is, in fact. */
        }
    }
    /* Prepare to the next: */
    lw_lmt = rdesc_p->base_address0 + rdesc_p->length0;
    if (!lw_lmt)
      break; /* We have reached the end of the physical address range. */
    rdesc_p = find_rdesc_lw(lw_lmt);
  }
}


#endif
