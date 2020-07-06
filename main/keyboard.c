/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_KEYBOARD_C
#define _MAIN_KEYBOARD_C 1


#include "../globals.h"

#include "keyboard.h"
#include "../asm/i386.h"
#include "../structures/buf.h"

#ifdef MAIN_STATIC_LINKED
#include "intr.c"
#include "system.c"
#else
#include "intr.h"
#include "system.h"
#endif

#ifdef CONFIG_KBD_8042


#define KBD8042_ENC_PORT 0x60
#define KBD8042_CTL_PORT 0x64

/* Keyboard controller state masks */
#define KBD8042_CTL_STAT_OUTBUF 0x01
#define KBD8042_CTL_STAT_INBUF  0x02
#define KBD8042_CTL_SELF_TEST   0x04
#define KBD8042_CTL_CMD_OR_DATA 0x08
#define KBD8042_CTL_KBD_LOCKED  0x10
#define KBD8042_CTL_OUTAUX_FULL 0x20
#define KBD8042_CTL_TIMEOUT     0x40
#define KBD8042_CTL_PARITY_ERR  0x80

#define GET_8042_KBD_DATA(data) inb(KBD8042_ENC_PORT,data)
#define PUT_8042_KBD_DATA(data) outb(KBD8042_ENC_PORT,data)
#define GET_8042_STATE(stat)    inb(KBD8042_CTL_PORT,stat)
#define PUT_8042_COMMAND(data)  outb(KBD8042_CTL_PORT,data)

static inline int test_8042_ctl_can_read(void)
{
  u8 stat;
  GET_8042_STATE(stat);
  return (int)stat & KBD8042_CTL_STAT_OUTBUF; /* Keyboard's output buffer is input from our point of view. */
}

static inline int test_8042_ctl_can_write(void)
{
  u8 stat;
  GET_8042_STATE(stat);
  return !((int)stat & KBD8042_CTL_STAT_INBUF); /* Keyboard's input buffer is output from our point of view. */
}

/*
 * Returns -1 if no char could be read from keyboard.
 * Otherwise, 8 lowest bits contains a byte read from
 * encoder output buffer.
 */
static inline int kbd8042_enc_read()
{
  int retc;
  u8 kbdc;
  if (test_8042_ctl_can_read())
  {
    GET_8042_KBD_DATA(kbdc);
    retc = kbdc;
  }
  else
    retc = -1;
  return retc;
}

/* Values written to encoding buffer: */
#define KBD8042_ENC_CMD_SET_LED          0xed
#define KBD8042_ENC_CMD_ECHO             0xee
#define KBD8042_ENC_CMD_SET_ALT_SET      0xf0
#define KBD8042_ENC_CMD_KBD_ID           0xf2
#define KBD8042_ENC_CMD_SET_REP_RATE     0xf3
#define KBD8042_ENC_CMD_KBD_ENABLE       0xf4
#define KBD8042_ENC_CMD_RESET_ENA        0xf5
#define KBD8042_ENC_CMD_RESET_SCAN       0xf6
#define KBD8042_ENC_CMD_PS2_AUTO_REP_ALL 0xf7
#define KBD8042_ENC_CMD_PS2_MAKE_BRK_ALL 0xf8
#define KBD8042_ENC_CMD_MAKE_ONLY_ALL    0xf9
#define KBD8042_ENC_CMD_MAK_BRK_AUTO_ALL 0xfa
#define KBD8042_ENC_CMD_AUTO_REP_SINGLE  0xfb
#define KBD8042_ENC_CMD_MAKE_BRK_SINGLE  0xfc
#define KBD8042_ENC_CMD_BRK_ONLY_SINGLE  0xfd
#define KBD8042_ENC_CMD_RESEND_LAST_RES  0xfe
#define KBD8042_ENC_CMD_HARD_RESET       0xff
/* ... and some helpfull flag masks: */
#define KBD8042_SCROLL_LOCK 0x00
#define KBD8042_NUM_LOCK    0x01
#define KBD8042_CAPS_LOCK   0x02

/* Values read from encoding buffer: */
#define KBD8042_ENC_RET_BUF_OVERRUN      0x00
/* 0x01..0x58, 0x81..0xd8 - are key scan codes. */
#define KBD8042_ENC_RET_TEST_OK          0xaa
#define KBD8042_ENC_RET_ECHO             0xee
#define KBD8042_ENC_RET_AT_PFX_CODE0     0xf0
#define KBD8042_ENC_RET_AKN              0xfa
#define KBD8042_ENC_RET_PS2_TEST_FAILED  0xfc
#define KBD8042_ENC_RET_AT_DIAGN_FAILED  0xfd
#define KBD8042_ENC_RET_RESEND_LAST_CMD  0xfe
#define KBD8042_ENC_RET_PS2_KEY_ERR      0xff

/* Values written to keyboard controller (standard commands only): */
#define KBD8042_CTL_CMD_READ_CMD         0x20
#define KBD8042_CTL_CMD_WRITE_CMD        0x60
#define KBD8042_CTL_CMD_SELF_TEST        0xaa
#define KBD8042_CTL_CMD_IFACE_TEST       0xab
#define KBD8042_CTL_CMD_KBD_DISABLE      0xad
#define KBD8042_CTL_CMD_KBD_ENABLE       0xae
#define KBD8042_CTL_CMD_READ_INPORT      0xc0
#define KBD8042_CTL_CMD_READ_OUTPORT     0xd0
#define KBD8042_CTL_CMD_WRITE_OUTPORT    0xd1
#define KBD8042_CTL_CMD_READ_TEST_IN     0xe0
#define KBD8042_CTL_CMD_SYSTEM_RESET     0xfe
#define KBD8042_CTL_CMD_MOUSE_PORT_DIS   0xa7
#define KBD8042_CTL_CMD_MOUSE_PORT_ENA   0xa8
#define KBD8042_CTL_CMD_MOUSE_PORT_TEST  0xa9
#define KBD8042_CTL_CMD_WRITE_TO_MOUSE   0xd4
/* ... and some usefull masks to generate command byte (see: 0x20, 0x60 controller commands) */
#define KBD8042_CMD_BYTE_KBD_INT_ENABLED       0x01
#define KBD8042_CMD_BYTE_PS2_MOUSE_INT_ENABLED 0x02
#define KBD8042_CMD_BYTE_WARM_REBOOT           0x04
#define KBD8042_CMD_BYTE_AT_IGNORE_LOCK        0x08
#define KBD8042_CMD_BYTE_KBD_DISABLE           0x10
#define KBD8042_CMD_BYTE_MOUSE_DISABLE         0x20
#define KBD8042_CMD_BYTE_TRANSLATION           0x40


/* Keyboard low-level input buffer for data directly send by keyboard via 8042. */
TYPEDEF_BUFFER(kbd8042_buf_t, KBD8042_BUF_LEN);
static kbd8042_buf_t kbd_llbuf;
static u8 kbd8042_pfx_stat = 0; /* 0x00, 0xe0 or 0xe1 */

/*
 * Gets next scan code (joined with E0 or E1 prefix, as well as with "unmake" bit).
 * Returns 0 if no code is available in the buffer.
 * (This function is not thread-safe.)
 */
static u16 get_next_8042_scan_code(void)
{
  /* Decl. */
  u16 sc = 0;
  char c;
  /* Do we have anything pending in the keyboard buffer? */
  if (!TEST_BUFFER_EMPTY(kbd_llbuf))
  {
    BUFFER_GET_CHAR(kbd_llbuf, c);
    if (kbd8042_pfx_stat == 0)
    {
      if ((u8)c == (u8)0xe0 || (u8)c == (u8)0xe1)
      {
        kbd8042_pfx_stat = 0x00ff & (u16)c;
        sc = get_next_8042_scan_code();
      }
      else
        sc = 0x00ff & (u16)c;
    }
    else
    {
      sc = (((u16)kbd8042_pfx_stat) << 8) + (0x00ff & (u16)c);
      kbd8042_pfx_stat = 0;
    }
  }
  return sc;
}

/*
 * This function still is not thread-safe, although it is secured
 * against keyboard interrupts.
 */
u16 get_next_8042_scan_code_noint(void)
{
  int int_on = test_int_on();
  int sc;
  if (int_on)
    cli();
  sc = get_next_8042_scan_code();
  if (int_on)
    sti();
  return sc;
}

/* IRQ #1 interrupt handler */
int inth_irq1kbd(int intno, u32 ierrno, u32 segm, u32 offset)
{
  int c;
  /* Read from keyboard. */
  while (test_8042_ctl_can_read() && !TEST_BUFFER_FULL(kbd_llbuf))
  {
    c = kbd8042_enc_read();
    if (c == -1)
      break;
    BUFFER_PUT_CHAR(kbd_llbuf,(char)c);
  }
  return 0;
}


key_t kbd8042_flags = 0x00000000; /* Flags if modifying keys being pressed at the moment. */
key_t buf_key       = 0x00000000; /* Buffered key. If 0 no key code is currently buffered. */

static key_t read_next_key()
{
  key_t ret = 0x00000000;
  u16 sc;
  while (!ret && !TEST_BUFFER_EMPTY(kbd_llbuf))
  {
    sc = get_next_8042_scan_code_noint();
    if (!sc)
      return 0x00000000; /* Input buffer empty. */
    switch (sc)
    {
      case 0x002a: /* + L-Shift */
        kbd8042_flags |= KEY_LSHFT;
        break;
      case 0x00aa: /* - L-Shift */
        kbd8042_flags &= ~KEY_LSHFT;
        break;
      case 0x001d: /* + L-Ctrl */
        kbd8042_flags |= KEY_LCTRL;
        break;
      case 0x009d: /* - L-Ctrl */
        kbd8042_flags &= ~KEY_LCTRL;
        break;
      case 0x0038: /* + L-Alt */
        kbd8042_flags |= KEY_LALT;
        break;
      case 0x00b8: /* - L-Alt */
        kbd8042_flags &= ~KEY_LALT;
        break;
      case 0xe038: /* + R-Alt */
        kbd8042_flags |= KEY_RALT;
        break;
      case 0xe0b8: /* - R-Alt */
        kbd8042_flags &= ~KEY_RALT;
        break;
      case 0xe01d: /* + R-Ctrl */
        kbd8042_flags |= KEY_RCTRL;
        break;
      case 0xe09d: /* - R-Ctrl */
        kbd8042_flags &= ~KEY_RCTRL;
        break;
      case 0x0036: /* + R-Shift */
        kbd8042_flags |= KEY_RSHFT;
        break;
      case 0x00b6: /* - R-Shift */
        kbd8042_flags &= ~KEY_RSHFT;
        break;
      case 0xe02a: /* + Q-Shift, - A-Shift */
        if (kbd8042_flags & KEY_ASHFT)
          kbd8042_flags &= ~KEY_ASHFT;
        else
          kbd8042_flags |= KEY_QSHFT;
        break;
      case 0xe0aa: /* - Q-Shift, + A-Shift */
        if (kbd8042_flags & KEY_QSHFT)
          kbd8042_flags &= ~KEY_QSHFT;
        else
          kbd8042_flags |= KEY_ASHFT;
        break;
      case 0xe11d: /* + Q-Ctrl */
        kbd8042_flags |= KEY_QCTRL;
        break;
      case 0xe19d: /* - Q-Ctrl */
        kbd8042_flags &= ~KEY_QCTRL;
        break;
      default:
        if (!(sc & KEY_UNMAKE)) /* If this is not an "unpressed" scan code... */
          return kbd8042_flags | (key_t)sc;
    }
  }
  return 0x00000000;
}

/*
 * Works out scan codes from the 8042 keyboard buffer. The result key codes are buffered if possible.
 * Return 0 if the 8042 keyboard buffer does not contain any key-pressed-like scan code.
 */
key_t get_next_key(void)
{
  /* Decl. */
  key_t ret = 0x00000000;
  /* Check buffer. */
  if (buf_key)
  {
    ret = buf_key;
    buf_key = 0x00000000;
  }
  else
    ret = read_next_key();
  buf_key = read_next_key();
  return ret;
}

/*
 * Main keyboard initialization routine.
 */
void init_8042(void)
{
  int res;
  
  /* Init data structures */
  INIT_BUFFER(kbd_llbuf);
  
  /* Wait for 8042 being ready to send command to it. Then, send "Enable keyboard" command to the controller. */
  while (!test_8042_ctl_can_write())
    ;
  PUT_8042_COMMAND(KBD8042_CTL_CMD_KBD_ENABLE);
  
  /* Set interrupt handler. */
  res = set_intr_hndl_force(OFFSET_8259A_IRQ_MASTER + 1,inth_irq1kbd);
}

#endif

#endif

