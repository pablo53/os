/*
 * (C) 2011 Pawel Ryszawa
 * Parallel programming.
 */

#ifndef _MAIN_SEMAPHORE_H
#define _MAIN_SEMAPHORE_H 1


#include "../globals.h"
#include "../asm/types.h"


/*
 * Defines a waiting-for-semaphore routine.
 * Any thread can WAIT FOR the same semaphore ONLY ONCE!
 * Such a thread MUST finally FREE all the semaphores it occupies and
 * CANNOT FREE any semaphore it had not overtaken previously.
 */
#define DEF_WAIT_SEM(sem_nm) __asm__ ( \
        ".global _semsect_wait_" #sem_nm " \n\t" \
        "_semsect_wait_" #sem_nm ": \n\t" \
        "mov $1,%eax \n\t" \
        "_semsect_loop_" #sem_nm ": \n\t" \
        "lock \n\t" \
        "xchg %eax,_semvar_" #sem_nm " \n\t" \
        "test %eax,%eax \n\t" \
        "jnz _semsect_loop_" #sem_nm " \n\t" \
        "ret \n\t" \
        )

/*
 * Defines a no-wait-but-check semaphore routine.
 * No thread will stop here. Instead, it will be informed about semaphore
 * being already occupied. Nevertheless, previously free semaphore will
 * be treaded as occupied by the thread calling this routine. If the semaphore
 * was previously occupied, it will not be treated as occupied by the current
 * thread on return from this routine.
 * This is a routine to utilize in code parts where, otherwise, there would be
 * possibilty of deadlocks.
 */
#define DEF_NOWAIT_SEM(sem_nm) __asm__ ( \
        ".global _semsect_nowait_" #sem_nm " \n\t" \
        "_semsect_nowait_" #sem_nm ": \n\t" \
        "mov $1,%eax \n\t" \
        "_semsect_loop2_" #sem_nm ": \n\t" \
        "lock \n\t" \
        "xchg %eax,_semvar_" #sem_nm " \n\t" \
        "ret \n\t" \
        )

/* Defines a named semaphore. */
/* Macro should be used outside any function. */
#define DEF_SEMAPHORE(sem_nm) \
  volatile u32 _semvar_##sem_nm; \
  DEF_WAIT_SEM(sem_nm); \
  DEF_NOWAIT_SEM(sem_nm)

/* Declares usage of a semaphore defined elsewhere. */
#define DECL_SEMAPHORE(sem_nm) \
  extern volatile u32 _semvar_##sem_nm 

/* Actual semaphore value (resolved to variable). */
#define SEMAPHORE(sem_nm) _semvar_##sem_nm

/* Actually waiting for free semaphore... */
#define WAIT_SEM(sem_nm) __asm__ __volatile__ ( \
        "call _semsect_wait_" #sem_nm " \n\t" \
        : \
        : \
        : "%eax" \
        )

/* Take over a free semaphore, but do not wait if it has already been occupied. Tell whether we should keep on waiting (semaphore already occupied). */
#define TRY_NOWAIT_SEM(sem_nm,keeponwait) __asm__ __volatile__ ( \
        "call _semsect_nowait_" #sem_nm " \n\t" \
        : "=a" (keeponwait) \
        : \
        )

/* Freeing semaphore previously overtaken with WAIT_SEM macro. */
#define FREE_SEM(sem_nm) __asm__ __volatile__ ( \
        "mov $0,%%eax \n\t" \
        "mov %%eax,_semvar_" #sem_nm " \n\t" \
        : \
        : \
        : "%eax" \
        )



DECL_SEMAPHORE(main_latch);


#endif

