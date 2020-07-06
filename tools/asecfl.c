/*
 * 2011 Pawel Ryszawa
 * Add an extra 512b block (sector size).
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


#define BLOCK_SIZE 512


static char nulltab[512] = { 0, };

static void die(const char * msg)
{
  fprintf(stderr, msg);
  exit(EXIT_FAILURE);
}

static void open_file(const char * fname, FILE ** f_p)
{
  int res;
  *f_p = fopen(fname, "a+");
  if (*f_p == NULL)
    die("Error opening file.");
  res = fseek(*f_p, 0, SEEK_END);
  if (res == -1)
    die("Error seeking end of file\n");
}

/*
 * Appends null characters.
 */
static void add_sector(FILE * f)
{
  int cnt;
  cnt = fwrite(nulltab, BLOCK_SIZE, 1, f);
  if (cnt != 1)
    die("Error adding an extra sector to the file\n");
}

/*
 * Main function. Entry point.
 */
int main(int argc, char * argv[])
{
  FILE * f;
  
  if (argc != 2)
    die("Usage: algnfl <FILE_NAME>\n");
  open_file(argv[1], &f);
  add_sector(f);
  fclose(f);
  return 0;
}
