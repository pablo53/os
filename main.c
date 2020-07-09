/*
 * 2011 Pawel Ryszawa
 * 
 */

#include "globals.h"

#include "asm/i386.h"
#include "asm/types.h"
#include "main.h"

static void _main(ptr_t ph_addr, ptr_t mmap_addr, msz_t mmap_length);

/*
 * This is the very first routine in C this system
 * enters to. System cannot return from it.
 * First parameter should pass base address of both
 * code and data segment.
 */
void _start(ptr_t ph_addr, ptr_t mmap_addr, u32 mmap_length)
{
  _main(ph_addr, mmap_addr, mmap_length);
  
  /* This function cannot return. */
  cli();
  while (1) {
    hlt();
  }
}

#include "main/all.h"


static int test_int3 = 0; /* Set on INT#3 test success. */

/*
 * Testing INT#3 interrupt handler.
 */
static int inth_3test(int intno, u32 ierrno, u32 segm, u32 offset)
{
  if (intno == 3 && ierrno == 0)
  {
    tprintk("[INT 0x03 Ok]");
    test_int3 = 1;
    return 0;
  }
  else
  {
    tprintfk("[INT 0x%2x: ERR=0x%8x]", intno, ierrno);
    return 1; /* Some error... but do not panic. */
  }
}

#ifdef CONFIG_PIC_8259A

static int inth_irq0test(int intno, u32 ierrno, u32 segm, u32 offset)
{
  int res;
  if (intno == OFFSET_8259A_IRQ_MASTER && ierrno == 0)
  {
    tprintfk("[INT 0x%2x Ok]", intno);
    FREE_SEM(main_latch);
    res = set_intr_hndl(intno, 0); /* We do not need this testing handler any more, anyway if cleaning fails - no problem. */
    return res;
  }
  else
  {
    tprintk("[INT 0x");
    tprinthexb((char)intno);
    tprintk(": ERR=0x");
    tprinthexb(ierrno);
    tprintk("]");
    return 1; /* Some error... but do not panic. */
  }
}

#endif

/*
 * Resets interrupts.
 */
static void stintrs(ptr_t ph_addr) {
  /* Init */
  int res = 0;

  /* Load IDT */
  tprintk("Loadint IDT...");
  init_idt(ph_addr);
  tprintk(" done.\n");
  
  tprintk("Testing \"breakpoint\" interrupt...");
  test_int3 = 0;
  res = set_intr_hndl(3, inth_3test);
  if (res != 0)
  {
    tprintk(" failed to set interrupt #3 handler!\n");
    kpanic();
  }
  __asm__ __volatile__ ("int $3");
  if (test_int3)
    tprintk(" done.\n");
  else
  {
    tprintk(" failed!\n");
    kpanic();
  }
  
  /* PIC initalization. */
#ifdef CONFIG_PIC_8259A
  tprintk("Starting master & slave 8259A Programmable Interrupt Controllers...");
  init_8259A();
  tprintk(" done.\n");
#endif

#ifdef CONFIG_APIC
  tprintk("APIC:");
  init_apic();
  if (lapic_pres)
    tprintfk(" Local APIC found at physical address 0x%8x.\n", (ptr_t)(lapic_reg + ph_addr));
  else
    tprintk(" not found.\n");
#endif
  
  /* PIT initalization. */
#ifdef CONFIG_PIT_8253
  tprintk("Starting 8253 Programmable Interval Timer...");
  init_8253();
  tprintk(" done.\n");
#endif
  
  /* Unmask interrupts with a processor flag. */
  tprintk("Unmasking interrputs...");
  sti();
  tprintk(" done.\n");

}

/*
 * System timer checking routine.
 * It is assumed that interrupts are already unmasked.
 */
static void chktimer()
{
#ifdef CONFIG_PIC_8259A
  int res;
  cli();
  tprintk("Setting testing timer handler...");
  res = set_intr_hndl(OFFSET_8259A_IRQ_MASTER + 0, inth_irq0test); /* IRQ #0 */
  if (res != 0)
  {
    tprintk(" failed to set interrupt handler!\n");
    kpanic();
  }
  tprintk(" done.\n");
  /* So... now, we are waiting for the first time to gain the main system latch. */
  /* This will be the IRQ#0 (INT #32) interrupt handler that frees it initially. */
  tprintk("Waiting for the first interrupt...");
  sti();
  WAIT_SEM(main_latch);
  tprintk(" done.\n");
#endif
  FREE_SEM(main_latch);
  tprintk("Main latch freed.\n");
}

static void sgmflt(void)
{
  init_sgmflth();
}

static void stkbd(void)
{
#ifdef CONFIG_KBD_8042
  tprintk("Keyboard...");
  init_8042();
  tprintk(" done.\n");
#endif
}

static void stdma(void)
{
#ifdef CONFIG_DMA_8237A_AT
  tprintk("Initializing 8237A Direct Memory Access controllers...");
  init_8237A();
  tprintk(" done.\n");
#endif
}

static void stmm()
{
  tprintk("Initializing memory manager...");
  init_mem_mgr();
  tprintk(" done.\n");
}

static void gtsinf(void)
{
  tprintk("CPU info:");
  init_cpu_info();
  if (cpuidsupp)
    tprintfk("%s, family=%i, model=%i, stepping id=%i\n", cpu_name, cpu_family, cpu_model, cpu_stepid);
  else
    tprintk(" not supported\n");
}

static void imem(ptr_t ph_addr, ptr_t mmap_addr, msz_t mmap_length)
{
  tprintfk("Memory map physical address: 0x%8x, length: 0x%4x.\n", mmap_addr, (u16)(mmap_length & 0x0000ffff));
  init_mem(ph_addr, mmap_addr, mmap_length);
  msz_t ram = get_mem_limit() + 1;
  ram = ram ? (ram >> 20) : 0x1000;
  tprintfk("RAM size: %i MiB\n", ram);
}

static void _main(ptr_t ph_addr, ptr_t mmap_addr, msz_t mmap_length)
{
  /* Say hello: */
  tmvcur(0, 7);
  tprintfk("C entry point _start() reached - code starts at physical address: 0x%x\n", ph_addr);

  /* First, remember physical address globally: */
  imem(ph_addr, mmap_addr, mmap_length);
  
  /* Get basic system info. */
  gtsinf();

  /* Initialize memory manager. */
  stmm();

  /* Initialize interrupt mechanism. */
  stintrs(ph_addr);

  /* Initialize general semaphore. */
  SEMAPHORE(main_latch) = 1;
  
  /* Let's check if the system timer interrupts periodically... */
  chktimer();
  
  /* Initialize segmentation fault handler. */
  sgmflt();
  
  /* Initialize keyboard. */
  stkbd();
  
  /* Initialize DMA */
  stdma();
}
