/*
 * Tools necessary for creating bootable CD (ISO-9660) image.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#include "util.h"

/*
 * Auxiliary section.
 */
 
void die(const char *msg, ...)
{
  fprintf(stderr, msg); /* Watch out! Stack overflow possibility. */
  exit(EXIT_FAILURE);
}

/*
 * Parse hex number.
 */
static int parse_hex(const char * snm)
{
  if (snm[0] == '0')
    if (snm[1] == 'x' || snm[1] == 'X')
      snm += 2;
  int ret_val = 0;
  while (snm[0] != '\000')
  {
    ret_val *= 16;
    if ((snm[0] >= '0') && (snm[0] <= '9'))
    {
      ret_val += (snm[0] - '0');
      snm++;
    }
    else if ((snm[0] >= 'a') && (snm[0] <= 'f'))
    {
      ret_val += (snm[0] - 'a' + 10);
      snm++;
    }
    else if ((snm[0] >= 'A') && (snm[0] <= 'F'))
    {
      ret_val += (snm[0] - 'A' + 10);
      snm++;
    }
    else
      die("Error parsing: not a naumber.\n");
  }
  return ret_val;
}

/*
 * Parse number.
 */
int parse_int(const char * snm)
{
  if (snm[0] == '0')
    if (snm[1] == 'x' || snm[1] == 'X')
      return parse_hex(snm + 2); /* This is hex number... */
  int ret_val = 0;
  while (snm[0] != '\000')
  {
    ret_val *= 10;
    if ((snm[0] >= '0') && (snm[0] <= '9'))
    {
      ret_val += (snm[0] - '0');
      snm++;
    }
    else
      die("Error parsing: not a naumber.\n");
  }
  return ret_val;
}

/*
 * All the destination buffers in the following functions
 * must be allocated memory!
 */
 
void both_byte_order8(char* dst_buf, char val)
{
  lsb_byte_order8(&dst_buf[0], val);
  msb_byte_order8(&dst_buf[1], val);
}

void both_byte_order16(char* dst_buf, short int val)
{
  lsb_byte_order16(&dst_buf[0], val);
  msb_byte_order16(&dst_buf[2], val);
}

void both_byte_order32(char* dst_buf, int val)
{
  lsb_byte_order32(&dst_buf[0], val);
  msb_byte_order32(&dst_buf[4], val);
}

void lsb_byte_order8(char* dst_buf, char val)
{
  dst_buf[0] = val;
}

void lsb_byte_order16(char* dst_buf, short int val)
{
  dst_buf[0] = (char)(val & 0xff);
  dst_buf[1] = (char)((val >> 8) & 0xff);
}

void lsb_byte_order32(char* dst_buf, int val)
{
  dst_buf[0] = (char)(val & 0xff);
  dst_buf[1] = (char)((val >> 8) & 0xff);
  dst_buf[2] = (char)((val >> 16) & 0xff);
  dst_buf[3] = (char)((val >> 24) & 0xff);
}

void msb_byte_order8(char* dst_buf, char val)
{
  dst_buf[0] = val;
}

void msb_byte_order16(char* dst_buf, short int val)
{
  dst_buf[1] = (char)(val & 0xff);
  dst_buf[0] = (char)((val >> 8) & 0xff);
}

void msb_byte_order32(char* dst_buf, int val)
{
  dst_buf[3] = (char)(val & 0xff);
  dst_buf[2] = (char)((val >> 8) & 0xff);
  dst_buf[1] = (char)((val >> 16) & 0xff);
  dst_buf[0] = (char)((val >> 24) & 0xff);
}
