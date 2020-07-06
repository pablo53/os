#ifndef _MAIN_INTR_H
#define _MAIN_INTR_H 1

#include "../globals.h"

#include "../asm/types.h"
#include "../asm/descriptor.h"

#define IDT_SIZE 256

#ifdef CONFIG_PIC_8259A

#define OFFSET_8259A_IRQ_MASTER 0x20
#define OFFSET_8259A_IRQ_SLAVE  0x28

#define IS_8259A_MASTER_INT(intno) ((intno) >= OFFSET_8259A_IRQ_MASTER && (intno) < OFFSET_8259A_IRQ_MASTER + 8)
#define IS_8259A_SLAVE_INT(intno) ((intno) >= OFFSET_8259A_IRQ_SLAVE && (intno) < OFFSET_8259A_IRQ_SLAVE + 8)

#define INT_IRQ0  32
#define INT_IRQ1  33
#define INT_IRQ2  34
#define INT_IRQ3  35
#define INT_IRQ4  36
#define INT_IRQ5  37
#define INT_IRQ6  38
#define INT_IRQ7  39
#define INT_IRQ8  40
#define INT_IRQ9  41
#define INT_IRQ10 42
#define INT_IRQ11 43
#define INT_IRQ12 44
#define INT_IRQ13 45
#define INT_IRQ14 46
#define INT_IRQ15 47

#endif

/* IDT type */
typedef gate_dsc_t idt_t[IDT_SIZE];

/* IDT quasi descriptor type */
typedef struct
{
  u16 limit;
  u16 lo_base_addr;
  u16 hi_base_addr;
} idt48_t;

/* Dynamically set table of 256 actual interrupt handlers. */
typedef int (*intr_hndl_t)(int, u32, u32, u32); /* Interrupt handler function returns <0 to panic and halt the system. */
typedef intr_hndl_t intr_hndl_tab_t[256];

/* IDT initialization function. */
void init_idt(ptr_t);

/* General interrupt handler. */
void inth(int, u32, u32, u32) __attribute__((noinline));

#ifdef CONFIG_PIC_8259A

/* PIC initalization. */
void init_8259A();

#endif

/* Setting actual interrupt handler. */
int set_intr_hndl(int intno, intr_hndl_t intr_hndl);
int set_intr_hndl_force(int intno, intr_hndl_t intr_hndl);

#endif
