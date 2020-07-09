/*
 * (C) 2011 Pawel Ryszawa
 * Interrupts handling.
 * This file is intedned to be included in /main.c only.
 */

#ifndef _MAIN_INTR_C
#define _MAIN_INTR_C 1


#include "../globals.h"

#include "intr.h"
#include "../asm/i386.h"
#include "../asm/segments.h"
#include "lterm.h"
#include "system.h"

volatile idt_t idt;  /* IDT */

volatile idt48_t idt_qdesc; /* IDT quasi descriptor for LIDT instruction. */

volatile intr_hndl_tab_t intr_hndl_tab = { 0, }; /* Actual interrupt handlers */


#ifdef CONFIG_PIC_8259A

/* Sending EOI to 8259A */
#define SEND_8259A_MASTER_EOI(irq_num) outb(0x20, 0x60 | (((char)(irq_num)) & 0x07))
#define SEND_8259A_SLAVE_EOI(irq_num) outb(0xa0, 0x60 | (((char)(irq_num)) & 0x07))

#endif

/*
 * Core interrupt handler.
 * Interrupt being handled, and error number (if put on the stack) are passed.
 */
void inth(int intno, u32 ierrno, u32 segm, u32 offset)
{
  int res = 0;
  if (intr_hndl_tab[intno])
  {
    res = intr_hndl_tab[intno](intno, ierrno, segm, offset);
    if (res < 0)
    {
      tprintfk("Interrupt 0x%8x handling routine failed.\n", intno);
      kpanic();
    }
  }
#ifdef CONFIG_PIC_8259A
  if (IS_8259A_MASTER_INT(intno))
    SEND_8259A_MASTER_EOI(intno - OFFSET_8259A_IRQ_MASTER);
  else if (IS_8259A_SLAVE_INT(intno))
    SEND_8259A_SLAVE_EOI(intno - OFFSET_8259A_IRQ_SLAVE);
#endif
}

/* Interrupt handler for interrupts not pushing error number on the stack. */
#define DEF_INTH(intno,hfaddr) __asm__( \
        "_inth_" #intno ": \n\t" \
        "push %ebp \n\t" \
        "mov %esp,%ebp \n\t" \
        "pusha \n\t" \
        "push %ds \n\t" \
        "push %es \n\t" \
        "push %fs \n\t" \
        "push %gs \n\t" \
        "mov $0x0048,%ax \n\t" \
        "mov %ax,%ds \n\t" \
        "mov %ax,%es \n\t" \
        "mov %ax,%fs \n\t" \
        "mov %ax,%gs \n\t" \
        "mov $0,%eax \n\t" \
        "mov 8(%ebp),%ebx \n\t" \
        "mov 12(%ebp),%edx \n\t" \
        "and $0x0000ffff,%edx \n\t" \
        "mov %esp,%ebp \n\t" \
        "sub $16,%esp \n\t" \
        "mov %ebx,%ss:-4(%ebp) \n\t" \
        "mov %edx,%ss:-8(%ebp) \n\t" \
        "mov %eax,%ss:-12(%ebp) \n\t" \
        "mov $" #intno ",%eax \n\t" \
        "mov %eax,%ss:-16(%ebp) \n\t" \
        "call " #hfaddr " \n\t" \
        "add $16,%esp \n\t" \
        "pop %gs \n\t" \
        "pop %fs \n\t" \
        "pop %es \n\t" \
        "pop %ds \n\t" \
        "popa \n\t" \
        "pop %ebp \n\t" \
        "iret \n\t" \
        )

/* Default interrupt handler for exceptions pushing error number on the stack. */
#define DEF_INTHE(intno,hfaddr) _DEF_INTHE(intno,hfaddr,GDT_MAIN_C_DATA)
#define _DEF_INTHE(intno,hfaddr,dseg) __asm__( \
        "_inth_" #intno ": \n\t" \
        "push %ebp \n\t" \
        "mov %esp,%ebp \n\t" \
        "pusha \n\t" \
        "push %ds \n\t" \
        "push %es \n\t" \
        "push %fs \n\t" \
        "push %gs \n\t" \
        "mov $0x0048,%ax \n\t" \
        "mov %ax,%ds \n\t" \
        "mov %ax,%es \n\t" \
        "mov %ax,%fs \n\t" \
        "mov %ax,%gs \n\t" \
        "mov 4(%ebp),%eax \n\t" \
        "mov 8(%ebp),%ebx \n\t" \
        "mov 12(%ebp),%edx \n\t" \
        "and $0x0000ffff,%edx \n\t" \
        "mov %esp,%ebp \n\t" \
        "sub $16,%esp \n\t" \
        "mov %ebx,%ss:-4(%ebp) \n\t" \
        "mov %edx,%ss:-8(%ebp) \n\t" \
        "mov %eax,%ss:-12(%ebp) \n\t" \
        "mov $" #intno ",%eax \n\t" \
        "mov %eax,%ss:-16(%ebp) \n\t" \
        "call " #hfaddr " \n\t" \
        "add $16,%esp \n\t" \
        "pop %gs \n\t" \
        "pop %fs \n\t" \
        "pop %es \n\t" \
        "pop %ds \n\t" \
        "popa \n\t" \
        "pop %ebp \n\t" \
        "add $4,%esp \n\t" \
        "iret \n\t" \
        )

/* Get interrupt handler entry point. */
#define INTH_ADDR(intno,haddr) __asm__ __volatile__ ( \
        "lea _inth_" #intno ",%%eax \t\n" \
        : "=a" (haddr) \
        : \
        )


/*
 * Setting routine for an interrupt.
 */
int set_intr_hndl(int intno, intr_hndl_t intr_hndl)
{
  /* First, test if interrupts are disabled. */
  if (test_int_on())
    return -1;
  /* Check if interrupt number is within correct range. */
  if (intno >= 0 && intno < 256)
  {
    intr_hndl_tab[intno] = intr_hndl;
    return 0;
  }
  else
    return -1;
}

/*
 * Setting routine for interrupt, masking interrupts for a while
 * if necessary (so, use it carefully!).
 */
int set_intr_hndl_force(int intno, intr_hndl_t intr_hndl)
{
  volatile int unmasked = test_int_on();
  int res;
  if (unmasked)
    cli();
  res = set_intr_hndl(intno, intr_hndl);
  if (unmasked)
    sti();
  return res;
}

gate_dsc_t static inline init_intrgate(ptr_t hfaddr)
{
  gate_dsc_t gate_dsc;
  gate_dsc.lo_offset = (u16)(0xffff & (u32)(hfaddr));
  gate_dsc.selector = GDT_MAIN_C_CODE;
  gate_dsc.number = 0;
  gate_dsc.desc_type = DESC_T_INTRGATE;
  gate_dsc.hi_offset = (u16)((0xffff0000 & (u32)(hfaddr)) >> 16);
  return gate_dsc;
}

gate_dsc_t static inline init_trapgate(ptr_t hfaddr)
{
  gate_dsc_t gate_dsc;
  gate_dsc.lo_offset = (u16)(0xffff & (u32)(hfaddr));
  gate_dsc.selector = GDT_MAIN_C_CODE;
  gate_dsc.number = 0;
  gate_dsc.desc_type = DESC_T_TRAPGATE;
  gate_dsc.hi_offset = (u16)((0xffff0000 & (u32)(hfaddr)) >> 16);
  return gate_dsc;
}


DEF_INTH(0,inth);
DEF_INTH(1,inth);
DEF_INTH(3,inth);
DEF_INTH(4,inth);
DEF_INTH(5,inth);
DEF_INTH(6,inth);
DEF_INTH(7,inth);
DEF_INTHE(8,inth);
DEF_INTH(9,inth);
DEF_INTHE(10,inth);
DEF_INTHE(11,inth);
//DEF_INTHE(12,inth); // TODO: should enter via a TSS
DEF_INTHE(13,inth);
DEF_INTHE(14,inth);
DEF_INTH(16,inth);
DEF_INTHE(17,inth);

#ifdef CONFIG_PIC_8259A

/* 8259A IRQs: */
DEF_INTH(32,inth); /* Master IRQ#0 Timer */
DEF_INTH(33,inth); /* Master IRQ#1 Keyboard */
DEF_INTH(34,inth); /* Master IRQ#2 (actually unused, this is input IRQ for Slave 8259A) */
DEF_INTH(35,inth); /* Master IRQ#3 */
DEF_INTH(36,inth); /* Master IRQ#4 */
DEF_INTH(37,inth); /* Master IRQ#5 */
DEF_INTH(38,inth); /* Master IRQ#6 */
DEF_INTH(39,inth); /* Master IRQ#7 */
DEF_INTH(40,inth); /* Slave IRQ#0 */
DEF_INTH(41,inth); /* Slave IRQ#1 */
DEF_INTH(42,inth); /* Slave IRQ#2 */
DEF_INTH(43,inth); /* Slave IRQ#3 */
DEF_INTH(44,inth); /* Slave IRQ#4 */
DEF_INTH(45,inth); /* Slave IRQ#5 */
DEF_INTH(46,inth); /* Slave IRQ#6 */
DEF_INTH(47,inth); /* Slave IRQ#7 */

#endif

/*
 * Setting IDT.
 * It is assumed that all interrupts are masked at the moment.
 */
void init_idt(ptr_t ph_addr)
{
  /* Init: */
  ptr_t hptr;
  
  /* IDT physical addresses */
  ptr_t idt_ph_addr = ((ptr_t)(&idt)) + ph_addr;
  
  /* Prepare IDT */
  for (int i = 0; i < IDT_SIZE; i++)
  {
    idt[i].lo_offset = 0x0000;
    idt[i].selector = 0x0000;
    idt[i].number = 0x00;
    idt[i].desc_type = 0x00;
    idt[i].hi_offset = 0x0000;
  }
#define INIT_TRAPGATE(intno) INTH_ADDR(intno,hptr); \
                             idt[intno] = init_trapgate(hptr)
#define INIT_INTRGATE(intno) INTH_ADDR(intno,hptr); \
                             idt[intno] = init_intrgate(hptr)
  INIT_TRAPGATE(0);
  INIT_TRAPGATE(1);
  INIT_TRAPGATE(3);
  INIT_TRAPGATE(4);
  INIT_TRAPGATE(5);
  INIT_TRAPGATE(6);
  INIT_TRAPGATE(7);
  INIT_TRAPGATE(8);
  INIT_TRAPGATE(9);
  INIT_TRAPGATE(10);
  INIT_TRAPGATE(11);
//  INIT_TRAPGATE(12); //TODO: Should use TSS gate.
  INIT_TRAPGATE(13);
  INIT_TRAPGATE(14);
  INIT_TRAPGATE(16);
  INIT_TRAPGATE(17);
#ifdef CONFIG_PIC_8259A
  INIT_INTRGATE(32); /* TIMER #0 should not interlace itself - hence interrupt (not trap) gate has been established. */
  INIT_INTRGATE(33); /* KEYBOARD should not interlace itself - hence interrupt (not trap) gate has been established. */
  INIT_TRAPGATE(34);
  INIT_TRAPGATE(35);
  INIT_TRAPGATE(36);
  INIT_TRAPGATE(37);
  INIT_TRAPGATE(38);
  INIT_TRAPGATE(39);
  INIT_TRAPGATE(40);
  INIT_TRAPGATE(41);
  INIT_TRAPGATE(42);
  INIT_TRAPGATE(43);
  INIT_TRAPGATE(44);
  INIT_TRAPGATE(45);
  INIT_TRAPGATE(46);
  INIT_TRAPGATE(47);
#endif
  
  /* Prepare IDT quasi (48-bit) descriptor */
  idt_qdesc.limit = IDT_SIZE * 8 - 1; /* IDT_SIZE (256) entries, 8-bytes long each. */
  idt_qdesc.lo_base_addr = 0x0000ffff & idt_ph_addr;
  idt_qdesc.hi_base_addr = (0xffff0000 & idt_ph_addr) >> 16;
  
  /* Set Interrupt Desciprtor Table (IDT): */
  lidt((ptr_t)&idt_qdesc);
}

#ifdef CONFIG_PIC_8259A

/*
 * 8259A PIC's initialization in x86 architecture.
 * It is assummed that all the interrupts are disabled at the moment (masked)!!!
 */
void init_8259A()
{
  outb(0x20, 0x11); /* ICW1 - master: init, edge triggered */
  outb(0xa0, 0x11); /* ICW1 - slave: init, edge triggered */
  outb(0x21, 0x20); /* ICW2 - master: offset (5 highest bits of IVB in bits 3..7), IVB's 32..39 */
  outb(0xa1, 0x28); /* ICW2 - slave: offset (5 highest bits of IVB in bits 3..7), IBV's 40..47 */
  outb(0x21, 0x20); /* ICW3 - master: the only slave PIC - connected to IRQ#2 */
  outb(0xa1, 0x20); /* ICW3 - slave: connected to IRQ#2 of its master */
  outb(0x21, 0x01); /* ICW4 - master: x86 architecture with 2 AKN signals from the processor */
  outb(0xa1, 0x01); /* ICW4 - slave: x86 architecture with 2 AKN signals from the processor */
}

#endif


#endif
