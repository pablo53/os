/*
 * (C) 2011 Pawel Ryszawa
 * IDE/ATAPI routines.
 */

#ifndef _DEV_IDE_C
#define _DEV_IDE_C 1


#include "../globals.h"

#include "ide.h"

#include "../asm/i386.h"
#include "../main/err.h"
#include "../main/time.h"
#include "../main/system.h"
#include "../main/semaphore.h"

#include <stdarg.h>

DEF_SEMAPHORE(dev_ide0); /* Semaphore for mutual exclusions of operations on IDE 0. */
DEF_SEMAPHORE(dev_ide1); /* Semaphore for mutual exclusions of operations on IDE 1. */

/*
 * Waiting for IDE channel until it shows "busy".
 * Return 0 on success, or non-zero if timed-out or another error occurs.
 * Be carefull: no additional parameters check are provided. So, passing
 * unexpected values can lead to unexpected results!
 */
static int ide_waiting_busy_loop(int ideno, int timeout, int timeout_ticks, int sleep, int canhalt)
{
  int busy;
  char status;
  do
  {
    inb(ideno ? IDE1_STATUS : IDE0_STATUS, status); /* Get status. */
    busy = (int)(status & IDE_STATUS_BUSY);
    if (busy)
    {
      if (sleep)
        ; //TODO: sleep here if requested so...
      else if (canhalt)
        soft_halt();
    }
  } while (busy && (!timeout || is_before_ticks(timeout_ticks))); /* Wait for the controller until it is busy. */
  return busy ? ERR_TIMEOUT : 0;
}

/*
 * Waiting for IDE channel until it does not show "ready".
 * Return 0 on success, or non-zero if timed-out or another error occurs.
 * Be carefull: no additional parameters check are provided. So, passing
 * unexpected values can lead to unexpected results!
 */
static int ide_waiting_nready_loop(int ideno, int timeout, int timeout_ticks, int sleep, int canhalt)
{
  int ready;
  char status;
  do
  {
    inb(ideno ? IDE1_STATUS : IDE0_STATUS, status); /* Get status. */
    ready = (int)(status & IDE_STATUS_DRDY);
    if (!ready)
    {
      if (sleep)
        ; //TODO: sleep here if requested so...
      else if (canhalt)
        soft_halt();
    }
  } while (!ready && (!timeout || is_before_ticks(timeout_ticks))); /* Wait for the controller until it is busy. */
  return ready ? 0 : ERR_TIMEOUT;
}

/*
 * Testing IDE channel for "data request" bit without any waiting.
 * Return non-zero if DRQ and 0 if nDRQ.
 * Be carefull: no additional parameters check are provided. So, passing
 * unexpected values can lead to unexpected results!
 */
static int ide_test_drq_now(int ideno)
{
  char status;
  int drq;
  inb(ideno ? IDE1_STATUS : IDE0_STATUS, status); /* Get status. */
  drq = (int)(status & IDE_STATUS_DRQ); /* Check DRQ (data request). */
  return drq;
}

/* Some macros easing functions' code: */

#define IDE_ENTER(ideno) \
  { \
    int no_sem; \
    switch (ideno) \
    { \
      case 0: \
        TRY_NOWAIT_SEM(dev_ide0, no_sem); \
        if (no_sem) \
          return ERR_PARALLEL_OPERATIONS; \
        break; \
      case 1: \
        TRY_NOWAIT_SEM(dev_ide1, no_sem); \
        if (no_sem) \
          return ERR_PARALLEL_OPERATIONS; \
        break; \
      default: \
        return ERR_BAD_PARAMETER; \
    } \
  }

#define IDE_RETURN(ideno,ret_code) \
  { \
    switch (ideno) \
    { \
      case 0: \
        FREE_SEM(dev_ide0); \
        break; \
      case 1: \
        FREE_SEM(dev_ide1); \
        break; \
    } \
    return (ret_code); \
  }

/*
 * Sends EIDE/ATAPI packet.
 * Flags ATAPI_OP_F_* can modify behavior.
 * Additional parameters depend on flags passed. As a general rule, the lower the value
 * of a flag (i.e. the more on the right its bit stays), the "earlier" its parameter.
 *   -ATAPI_OP_F_TIMEOUT: Additional parameter is of type int and sets time-out for the operation in msec.
 *   -ATAPI_OP_F_SLEEP: Additional parameter is of type int and sets sleeping time while device is busy.
 *   -ATAPI_OP_F_CANHALT: Cannot be used with ATAPI_OP_F_SLEEP and vice-versa. Do not expect any parameters.
 *                        It allows to halt the processor while waiting for the device, provided that interrupts
 *                        are not masked (via x86 CLI instruction).
 */
int atapi_send_packet(int idedrv, atapi_pkt_t * atapi_pkt, int op_flags, ...)
{
  /* IDE 0 or IDE 1? Master or slave? */
  int ideno = IDEDRV_TO_IDENO(idedrv);
  int drvno = IDEDRV_TO_DRVNO(idedrv);
  int timeout = 0; /* Boolean: Is time-out set? */
  u32 timeout_ticks; /* 0 means "no timeout", otherwise - limit expressed as system's "ticks" value. */
  int sleep = 0; /* 0 means "cannot sleep" while waiting for hardware to response. */
  int canhalt = 0; /* Boolean: a non-zero value means "halt and wait for the next tick (interrupt)" for each busy/not ready cycle. */
  char status; /* Auxiliary variable. */
  /* Check flags: */
  if (op_flags)
  {
    /* Start iterating through additional parameters. */
    va_list arg;
    va_start(arg, op_flags);
    /* Check flags and read additional parameters accordingly. */
    if (op_flags & ATAPI_OP_F_TIMEOUT)
    {
      timeout_ticks = count_ticks_limit(va_arg(arg, int)); /* What is the "ticks" value for time-out to be assumed. */
      timeout = 1; /* Mark "time-out" flag. */
      op_flags &= ~ATAPI_OP_F_TIMEOUT; /* Unset this flag - we can do wothout it, time-out value is enough */
    }
    if (op_flags & ATAPI_OP_F_SLEEP)
    {
      sleep = va_arg(arg, int);
      op_flags &= ~ATAPI_OP_F_SLEEP; /* Unset this flag - we can do wothout it. */
    }
    if (op_flags & ATAPI_OP_F_CANHALT)
    {
      if (!sleep)
        return ERR_BAD_PARAMETER; /* Cannot sleep and halt at the same time. */
      canhalt = 1;
    }
    /* Finish iteration through additional parameters. */
    va_end(arg);
    /* Check if there are unknown flags left: */
    if (op_flags)
      return ERR_BAD_PARAMETER;
  }
  /* Additional checks: */
  if (sleep) //TODO
    return ERR_NOT_IMPLEMENTED;
  /* Configure ports and some values to be written to those ports: */
  int ide_data_reg = ideno ? IDE1_DATA_REG : IDE0_DATA_REG;
  int ide_drv_head = ideno ? IDE1_DRV_HEAD : IDE0_DRV_HEAD;
  int ide_command = ideno ? IDE1_COMMAND : IDE0_COMMAND;
  int ide_alt_status = ideno ? IDE1_ALT_STATUS : IDE0_ALT_STATUS;
  int drv_head = drvno ? IDE_DRV_HEAD_SLAVE : IDE_DRV_HEAD_MASTER;
  int ide_dev_ctl = ideno ? IDE1_DEV_CTL : IDE0_DEV_CTL;
  /* Critical section starts here, so mutual exclusion mechanism are going to be used: */
  IDE_ENTER(ideno); /* This macro can return from function with error if this IDE channel is currently in use. */
  /* Wait until the IDE channel is busy and then until it is not ready: */
  int res;
  if ((res = ide_waiting_busy_loop(ideno, timeout, timeout_ticks, sleep, canhalt)))
    IDE_RETURN(ideno, res); /* Operation timed-out. */
  if ((res = ide_waiting_nready_loop(ideno, timeout, timeout_ticks, sleep, canhalt)))
    IDE_RETURN(ideno, res); /* Operation timed-out. */
  /* Select drive: */
  outb(ide_drv_head, drv_head); /* This is the actual drive selection. */
  /* Disable device's interrupt: */
  outb(ide_dev_ctl, IDE_DEV_CTL | IDE_DEV_CTL_NIEN);
  /* Send "packet" ATA command: */
  outb(ide_command, IDE_COMMAND_ATAPI_PKT);
  loop_interval_u(1); /* Wait for 1 us (which is more than enough as compared to the ATAPI specification - 400 ns.) */
  /* Wait until the IDE channel is busy: */
  if ((res = ide_waiting_busy_loop(ideno, timeout, timeout_ticks, sleep, canhalt)))
    IDE_RETURN(ideno, res); /* Operation timed-out. */
  /* Check DRQ: */
  if (!ide_test_drq_now(ideno))
    IDE_RETURN(ideno, ERR_STATE); /* Drive should have required data from us. */
  /* Send package: */
  for (int i = 0; i < atapi_pkt->tab_len; i++)
    outb(ide_data_reg, atapi_pkt->tab[i]);
  /* The package has been sent. According to the ATAPI specification we must do a dummy cycle: */
  inb(ide_alt_status, status); /* We ignore this status, of course. */
  /* Wait for the controller not being busy, yet another time: */
  if ((res = ide_waiting_busy_loop(ideno, timeout, timeout_ticks, sleep, canhalt)))
    IDE_RETURN(ideno, res); /* Operation timed-out. */
  /* Everything seems to be ok - the caller must check DRQ and send data if necessary immediately after we return. */
  IDE_RETURN(ideno, 0);
}

/*
 * Returns the ATAPI packet size for the given device.
 */
int atapi_get_pkt_len(int idedrv)
{
  return 12; //TODO: This is the default, but 16 may also occur. 0 should be returned if the device is not ATAPI.
}

#endif

