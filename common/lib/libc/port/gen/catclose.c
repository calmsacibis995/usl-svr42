/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/catclose.c	1.5"

#ifdef __STDC__
	#pragma weak catclose = _catclose
#endif
#include "synonyms.h"
#include <dirent.h>
#include <stdio.h>
#include <nl_types.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

extern int munmap();
extern int _mmp_opened;

catclose(catd)
  nl_catd catd;
{
char symb_path[MAXNAMLEN];
char old_locale[MAXNAMLEN];

  if ((int)catd != -1 || catd == (nl_catd)NULL) {
    
    if (catd->type == MALLOC) {
        free(catd->info.m.sets);
	catd->type = -1;
	return 0;
    } else
    if (catd->type == MKMSGS) {
        munmap(catd->info.g.sets, catd->info.g.set_size);
        munmap(catd->info.g.msgs, catd->info.g.msg_size);
	catd->type = -1;
	_mmp_opened--;
	return 0;
    }
  }
  /*
   * was a bad catd
   */
  return -1;
}
