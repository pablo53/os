/*
 * (C) 2011 Pawel Ryszawa
 *
 * State machine mechanisms.
 */

#ifndef _STRUCTURES_STMACH_H
#define _STRUCTURES_STMACH_H 1


/*
 * example:
 *   TYPEDEF_STATE_MACHINE(kbd_mach_t,1024);
 *   void foo(void)
 *   {
 *     kbd_mach_t kbd_mach;
 *     INIT_STATE_MACHINE(kbd_mach);
 *     while (TEST_CORRECT_STATE(kbd_mach, kbd_mach.state))
 *       NEXT_STATE(get_next_char());
 *   }
 */

#define STMACH_INITIAL_STATE 0

/*
 * Type defining action undertaken on move between states.
 */
typedef void (*stmach_action_t)(u8, int, int); /* signal, previous state, next state */

/*
 * Defining a state machine type with the number of states and
 * signals, as well as signal type, given.
 */
#define TYPEDEF_STATE_MACHINE(typename,states) \
  typedef int stmchln_##typename[256]; \
  typedef stmach_action_t stmchact_##typename[256]; \
  typedef struct \
  { \
    int state; /* Current state. Very important: state 0 is always the initial state! */ \
    int num_states; /* Number of possible states. */ \
    stmchln_##typename trans[states]; /* Table of lines (hence matrix) of transitions. Each line concerns previous state, its elements - next possible states, indices - input signals. */ \
    stmchact_##typename actions[states]; /* Table of actions (hence matrix) undertaken between two states. */ \
  } typename

/*
 * State machine constructor routine.
 */
#define INIT_STATE_MACHINE(stmach) \
  stmach.state = 0; \
  stmach.num_states = sizeof(stmach.trans) / sizeof(int) / 256; \
  for (int i = 0; i < stmach.num_states; i++) \
    for (int j = 0; j < 256; j++) \
    { \
      stmach.trans[i][j] = STMACH_INITIAL_STATE; \
      stmach.actions[i][j] = 0; \
    }

/*
 * Tests, whether the state machine is in the correct state.
 */
#define TEST_CORRECT_STATE(stmach,statno) (statno >= 0 && statno < stmach.num_states)

/*
 * Move to the next state, doing an action between them.
 */
#define NEXT_STATE(stmach,signal) \
  if (stmach.actions[stmach.state][signal] != 0) \
    stmach.actions[stmach.state][signal](signal, stmach.state, stmach.trans[stmach.state][signal]); \
  stmach.state = stmach.trans[stmach.state][signal]


#endif
