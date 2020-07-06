/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_SEMAPHORE_C
#define _MAIN_SEMAPHORE_C 1


#include "../globals.h"
#include "semaphore.h"


/* This is a widely blocking semaphore - use it carefully! */
/* It must be cleared before first use. */
DEF_SEMAPHORE(main_latch);


#endif
