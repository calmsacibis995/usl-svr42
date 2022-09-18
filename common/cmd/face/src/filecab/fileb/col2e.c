/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */
/*	Copyright (c) 1992 Siemens Nixdorf Informationssysteme AG */
/*	All Rights Reserved  	*/

#ident	"@(#)face:src/filecab/fileb/col2e.c	1.1"

/* translate national colour name to english colour name

   usage: col2e <colour name>  --> <english colour name>

   exit:  0 if success, >0 if no success
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#define NCOL 8

/* ATTENTION: the indices 63,... are cat indices of uxface !!! */

static char *colours[NCOL] =
{
  "63\0black",
  "64\0blue",
  "65\0cyan",
  "66\0green",
  "67\0magenta",
  "68\0red",
  "69\0white",
  "70\0yellow"
};

void main(int argc,char **argv)
{
  int i;
  char *p,*q;
  char catidx[12];

  if (argc != 2) exit(1);
  strcpy(catidx,"uxface:");
  setlocale(LC_MESSAGES,"");
  for (i=0; i < NCOL; i++)
  {
    strcpy(catidx + 7, colours[i]);
    q=strchr(colours[i],0)+1;
    p=gettxt(catidx,q);
    if (!p) exit(2);
    if (strcoll(p,argv[1])==0) break;
  }
  if (i >= NCOL) exit(3);
  printf("%s\n",q);
  exit(0);
}


