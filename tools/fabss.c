/*
 * 2011 Pawel Ryszawa
 * Pad file so that it contained .BSS segment.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "util.h"


static char * nulltab;


static void open_file(const char * fname, FILE ** f_p)
{
  int res;
  *f_p = fopen(fname, "a+");
  if (*f_p == NULL)
    die("Error opening file.\n");
  res = fseek(*f_p, 0, SEEK_END);
  if (res == -1)
    die("Error seeking end of file\n");
}

/*
 * Appends num null characters.
 */
static void pad_file(FILE * f, int num)
{
  int cnt;
  cnt = fwrite(nulltab, num, 1, f);
  if (cnt != 1)
    die("Error padding file\n");
}

/*
 * Main function. Entry point.
 */
int main(int argc, char * argv[])
{
  FILE * f;
  long fsize;
  int pad;
  int bss_start, bss_length;
  
  if (argc != 4)
    die("Usage: fabss <FILE_NAME> <BSS_START> <BSS_LENGTH>\n");
  bss_start = parse_int(argv[2]);
  bss_length = parse_int(argv[3]);
  open_file(argv[1], &f);
  fsize = ftell(f);
  if (fsize == -1)
    die("Error determining file size.\n");
  pad = bss_start + bss_length - 1 - fsize;
  if (pad <= 0)
  {
    printf("BSS segment has already been included in the file.\n");
    return 0;
  }
  nulltab = (char *)malloc(pad + 1);
  if (!nulltab)
    die("Error allocating memory for buffer.\n");
  pad_file(f, pad);
  free((void *)nulltab);
  fclose(f);
  return 0;
}
