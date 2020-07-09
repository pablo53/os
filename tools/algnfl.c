/*
 * 2011 Pawel Ryszawa
 * Pad file so that it contains full blocks (of sector size or whatever...).
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

#define BLOCK_SIZE 512


static char * nulltab;

/*
static void die(const char * msg)
{
  fprintf(stderr, msg);
  exit(EXIT_FAILURE);
}
*/

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
  if (num == 0)
  {
    printf("No padding needed.\n");
    return;
  }
  cnt = fwrite(nulltab, num, 1, f);
  if (cnt != 1)
    die("Error padding file\n");
}

static int count_pad(long fsize, int blksz)
{
  int r = (int)(fsize % blksz);
  if (r != 0)
    r = blksz - r;
  return r;
}

/*
static int get_blksz(char * salgn)
{
  int blksz = 0;
  while (salgn[0] != '\000')
  {
    blksz *= 10;
    if ((salgn[0] >= '0') && (salgn[0] <= '9'))
    {
      blksz += (salgn[0] - '0');
      salgn++;
    }
    else
      die("Number expected as parameter.\n");
  }
  return blksz;
}
*/

/*
 * Main function. Entry point.
 */
int main(int argc, char * argv[])
{
  FILE * f;
  long fsize;
  int pad;
  int blksz = BLOCK_SIZE;
  
  if ((argc != 2) && (argc != 3))
    die("Usage: algnfl <FILE_NAME> [<ALIGN_POWER_OF_2>]\n");
  open_file(argv[1], &f);
  fsize = ftell(f);
  if (fsize == -1)
    die("Error determining file size.\n");
  if (argc ==3)
    blksz = 1 << parse_int(argv[2]); /*get_blksz(argv[2])*/
  pad = count_pad(fsize, blksz);
  nulltab = (char *)malloc(pad + 1);
  pad_file(f, pad);
  free((void *)nulltab);
  fclose(f);
  return 0;
}
