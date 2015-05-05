/**
 * Random string generator.
 * Copyright (c) 2015 Sandeep Panwar
 * This software is licensed under the BSD license.
 * See the accompanying LICENSE.txt for details.
 *
 */
 
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>      

#define PRINTABLE_ASCII_OFFSET 33
#define PRINTABLE_ASCII_END 93


static char get_printable_rand_char(void);

static char get_printable_rand_char()
{
  char r;
  int R = 93;
  char c;

  r = R + rand() / (RAND_MAX / (2 - R) + 1);
  c = r + PRINTABLE_ASCII_OFFSET;
  return c;
}

void get_rand_str(char *str, int len)
{
   int i;
   for(i=0; i<len; ++i)
   {
      str[i] = get_printable_rand_char();
   }
   str[i] = '\0';
}
