/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtedit:wrap.c	1.2"
#endif

/*
 * wrap.c
 *
 */

#include <stdio.h>

#define DEFAULT_TAB_AMOUNT     8
#define DEFAULT_LINE_LIMIT    72

typedef enum { SOFT, HARD } WrapType;

char   buffer[2 * BUFSIZ];

/*
 * main
 *
 * This module wraps lines read from standard input at
 * a given line limit.  In doing so, tabs, blanks, and
 * newlines are considered white space.  Wraps are made
 * at the last white space before the word that extends
 * beyond the line limit.  Tabs are expanded to a given
 * tab stop.  Defaults for the parameters governing the
 * algorithm are 72 character lines and 8 character tab
 * stops.  These can be overridden on the command line.
 *
 * Input is buffered until the line limit is exceeded.
 * The variable \fIwrap_point\fP is used to hold the
 * last known position where a wrap seems reasonable.
 * This is defined as whenever a blank or tab is found
 * or when a non-white space character is found after
 * a white space character.  This latter case handles
 * the situation when the character that forces the
 * wrap is white space.  In this case, we want to avoid
 * emitting the space at the beginning of the next line.
 *
 * This algorithm does not handle the case where the
 * blank or tab that forces a wrap is followed by more
 * blanks or tabs.  In this case the blank or tab that
 * forced the wrap is not printed, but the following
 * blanks or tabs are and they appear at the beginning
 * of the next line.
 *
 * Whenever a newline is encountered, the buffer is flushed.
 * In the event that the file ends with characters in the
 * buffer (i.e., the last character in the input is not
 * a newline), the buffer is flushed as well.
 *
 * To increase the efficiency of the module, in-line loops
 * are used to write the output and to copy strings.
 *
 * Usage:
 *
 * wrap [-w line_width] [-t tab_stop_width] < input
 *
 */

main(argc, argv)
int argc;
char * argv[];
{
   char *        p            = buffer;
   char *        wrap_point   = buffer;
   int           c;

   int           tab_amount   = DEFAULT_TAB_AMOUNT;
   int           line_limit   = DEFAULT_LINE_LIMIT;

   int           space_to_add;
   WrapType      type_of_wrap = HARD;

   int           optval;
   extern char * optarg;

   while ((optval = getopt(argc, argv, "w:t:")) != EOF)
   {
      switch (optval)
      {
         case 'w': line_limit = atoi(optarg);
                   break;

         case 't': tab_amount = atoi(optarg);
                   break;
         default:
            break;
      }
   }

   if (line_limit > BUFSIZ)
      line_limit = BUFSIZ;

   if (tab_amount >= line_limit)
      tab_amount = DEFAULT_TAB_AMOUNT;

   while ((c = getchar()) != EOF)
   {
      *p = c;

      switch (c)
      {
         case '\t': if ((space_to_add = tab_amount - ((p - buffer) % tab_amount)))
                       while (--space_to_add)
                          *p++ = ' ';
                    *p = ' ';
                    /* FALL THROUGH */
         case  ' ': wrap_point = p;
                    break;

         case '\n': wrap_point = p;
                    if (p == buffer && type_of_wrap == SOFT)
                    {
                       p--;
                       type_of_wrap = HARD;
                    }
                    else
                       p = &buffer[line_limit];
                    break;

         default:   if (wrap_point == buffer && (p - buffer) >= line_limit)
                       wrap_point = p;
                    else
                       if (*wrap_point == ' ')
                          wrap_point = p;
                    break;
      }

      if ((p - buffer) >= line_limit)
      {
         type_of_wrap = *wrap_point == '\n' ? HARD : SOFT;

         for (p = buffer; p < wrap_point; p++)
            putchar(*p);
         putchar('\n');

         if (*wrap_point == '\n' || *wrap_point == ' ')
            p = buffer;
         else
         {
            for (p = buffer; *wrap_point && *wrap_point != ' '; p++, wrap_point++)
               *p = *wrap_point;
            *p = '\0';
         }
         wrap_point = buffer;
      }
      else
         p++;
   }

   if (p != buffer)
   {
      *p = '\0';
      printf("%s", buffer);
   }

} /* end of main */
