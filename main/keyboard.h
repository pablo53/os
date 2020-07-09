/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _MAIN_KEYBOARD_H
#define _MAIN_KEYBOARD_H 1


#include "../globals.h"

#include "../asm/types.h"
#include "../structures/stmach.h"

#ifdef CONFIG_KBD_8042

#define KBD8042_BUF_LEN 16

typedef u32 key_t; /* Keycode container. Upper 16-bits are current flags KBD_xxxx. */

/*
 * Modifier keys flags (shifts, alts, ctrls, etc.). Qxxxx (quasi-xxxx) are generated autmatically
 * by the keyboard, so user cannot press it explicitly. Axxxx (anti-Qxxxx) are somehow "opposite"
 * to the corresponding Qxxxx - these cannot be set both at the same time, since Axxxx means that
 * the key Qxxxx was first "unpressed" before it was finally pressed (it means that "make code"
 * and "unmake code" were inversed in time).
 */
#define KEY_LALT  0x00010000
#define KEY_RALT  0x00020000
#define KEY_LCTRL 0x00040000
#define KEY_RCTRL 0x00080000
#define KEY_QCTRL 0x00100000
#define KEY_LSHFT 0x00200000
#define KEY_RSHFT 0x00400000
#define KEY_QSHFT 0x00800000
#define KEY_ASHFT 0x01000000
#define KEY_CPLCK 0x02000000
#define KEY_NMLCK 0x04000000
#define KEY_SCLCK 0x08000000

#define KEY_UNMAKE 0x0080

void init_8042(void);
u16 get_next_8042_scan_code_noint(void); /* Gets hardware key code, prefixed with E0 or E1 if necessary and with "unmake" bit. */
key_t get_next_key(void); /* Gets the next key code with appropriate flags, 0 if buffer empty. */

#endif


#endif

