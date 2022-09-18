/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:value.c	1.1"
/* value.c:
 *
 * routines for converting byte values to long values.  these are pretty
 * portable although they are not necessarily the fastest things in the
 * world.
 *
 * jim frost 10.02.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"

unsigned long memToVal(p, len)
     byte         *p;
     unsigned int  len;
{ unsigned int  a;
  unsigned long i;

  i= 0;
  for (a= 0; a < len; a++)
    i= (i << 8) + *(p++);
  return(i);
}

void valToMem(val, p, len)
     unsigned long  val;
     byte          *p;
     unsigned int   len;
{ int a;

  for (a= len - 1; a >= 0; a--) {
    *(p + a)= val & 0xff;
    val >>= 8;
  }
}

unsigned long memToValLSB(p, len)
     byte         *p;
     unsigned int  len;
{ int val, a;

  val= 0;
  for (a= len - 1; a >= 0; a--)
    val= (val << 8) + *(p + a);
  return(val);
}

/* this is provided for orthagonality
 */

void valToMemLSB(val, p, len)
     byte          *p;
     unsigned long  val;
     unsigned int   len;
{
  while (len--) {
    *(p++)= val & 0xff;
    val >>= 8;
  }
}
