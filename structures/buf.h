/*
 * (C) 2011 Pawel Ryszawa
 */

#ifndef _STRUCTURES_BUF_H
#define _STRUCTURES_BUF_H 1

/*
 * example:
 *   TYPEDEF_BUFFER(kbd_buf_t,256);
 *   void foo(void)
 *   {
 *     kbd_buf_t kbd_buf;
 *     INIT_BUFFER(kbd_buf);
 *   }
 */

#define TYPEDEF_BUFFER(buf_t,buf_max_len) \
  typedef struct \
  { \
    char data[buf_max_len]; \
    int max; \
    int startpos; \
    int endpos; \
  } buf_t

#define INIT_BUFFER(buf) \
  buf.max = sizeof(buf.data); \
  for (int i = 0; i < buf.max; i++) \
    buf.data[i] = 0; \
  buf.startpos = 0; \
  buf.endpos = 0

#define BUFFER_LENGTH(buf) ((buf.endpos - buf.startpos) + ((buf.endpos < buf.startpos) ? buf.max : 0))

#define TEST_BUFFER_EMPTY(buf) (buf.startpos == buf.endpos)

#define TEST_BUFFER_FULL(buf) ((buf.startpos == buf.endpos + 1) || ((buf.startpos == 0) && (buf.endpos == buf.max - 1)))

#define BUFFER_PUT_CHAR(buf,c) \
  buf.data[buf.endpos++] = c; \
  buf.endpos %= buf.max

//  buf.data[buf.endpos = buf.endpos + 1 - ((buf.endpos >= sizeof(buf.data) - 1) ? sizeof(buf.data) : 0)] = c

#define BUFFER_GET_CHAR(buf,c) \
  c = buf.data[buf.startpos++]; \
  buf.startpos %= buf.max

//c = buf.data[buf.startpos = buf.startpos + 1 - ((buf.startpos >= sizeof(buf.data) - 1) ? sizeof(buf.data) : 0)]

#endif
