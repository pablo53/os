/*
 * (C) 2011 Pawel Ryszawa
 *
 * A tool formatting file size.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

/* Die for not being used correctly. */
void die_args(void)
{
  die("Usage: ffsize <OPTION> <FILE_NAME> <FORMAT_STRING>\n\toptions:\n\t -s<SECTOR_SIZE>\n\t -a<ALIGN_BYTES>\n");
}

/*
 * Get file size.
 */
long get_file_size(const char * fname)
{
  FILE * f;
  int res;
  long fsz;
  
  f = fopen(fname, "r");
  if (f == NULL)
    die("Cannot open file.\n");
  res = fseek(f, 0, SEEK_END);
  if (res == -1)
    die("Error reading file.\n");
  fsz = ftell(f);
  if (fsz == -1)
    die("Error determining file size.\n");
  fclose(f);
  return fsz;
}

/*
 * Get sector size from a provided parameter.
 */
int get_sector_size(const char * sopt)
{
  if (strlen(sopt) < 3)
    die_args();
  if ((sopt[0] != '-') || ((sopt[1] != 's') && (sopt[1] != 'a')))
    die_args();
  return parse_int(&sopt[2]);
}

/*
 * Parse formatting string.
 */
char * parse_fmt_str(const char * fmts)
{
  /* Init. */
  int escs = 0;
  char * parsed;
  /* Source string length. */
  int slen = strlen(fmts);
  int c = 0; /* Source string cursor. */
  int d = 0; /* Destination string cursor. */
  /* Count escape characters. */
  while (c < slen)
  {
    if ((fmts[c] == '\\') && (c < slen - 1)) /* Last character cannot escape... */
    {
      escs++;
      c++; // Next char can never be escape character.
    }
    c++;
  }
  /* Allocate memory */
  parsed = malloc(slen - escs + 1); /* Do not forget to allocate memory for terminating null. */
  /* Do the string. */
  c = 0;
  while (c < slen)
  {
    if ((fmts[c] == '\\') && (c < slen - 1)) /* Last character cannot escape... */
    {
      c++;
      switch (fmts[c])
      {
        case 'n':
          parsed[d++] = '\n';
          break;
        case 'r':
          parsed[d++] = '\r';
          break;
        case 't':
          parsed[d++] = '\t';
          break;
        default:
          parsed[d++] = fmts[c];
      }
      c++;
    }
    else
    {
      parsed[d++] = fmts[c];
      c++;
    }
  }
  parsed[d] = '\000'; /* Terminating null. */
  return parsed;
}

/*
 * Main routine.
 */
int main(int argc, char * argv[])
{
  long fsz;
  int ssz;
  int cnt = 0;
  char * pstr;

  /* Check number of parameters: */
  if (argc != 4)
    die_args();
  /* Do the core job.*/
  ssz = get_sector_size(argv[1]);
  fsz = get_file_size(argv[2]);
  switch (argv[1][1])
  {
    case 's':
      cnt = (fsz / ssz) + (fsz % ssz > 0);
      break;
    case 'a':
      cnt = ssz * ((fsz / ssz) + (fsz % ssz > 0));
      break;
  }
  pstr = parse_fmt_str(argv[3]);
  printf(pstr, cnt); /* Watch out: buffer overrflow danger!!! */
  free(pstr);
  /* Everythong is ok. */
  exit(EXIT_SUCCESS);
}
