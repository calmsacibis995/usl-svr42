/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/catgets.c	1.7"

#ifdef __STDC__
	#pragma weak catgets = _catgets
#endif
#include "synonyms.h"
#include <dirent.h>
#include <locale.h>
#include <stdio.h>
#include <nl_types.h>
#include <string.h>
#include <pfmt.h>
#define min(a,b)  (a>b?b:a)

#ifdef __STDC__
char *
catgets(nl_catd catd, int set_num, int msg_num, const char *s)
#else
char *
catgets(catd, set_num, msg_num, s)
  nl_catd catd;
  int set_num, msg_num;
  char *s;
#endif
{
  register int i;
  int first_msg;
  int message_no;
  int msg_ptr;

  char *data;
  char *msg;
  char *p;

  struct cat_set_hdr *sets;
  struct cat_msg_hdr *msgs;
  struct m_cat_set *set;
  
  if ( (int)catd == -1 || catd == (nl_catd)NULL)
    return (char *)s;

  switch (catd->type) {
    case MALLOC :
      /*
       * Locate set
       */
      sets = catd->info.m.sets;
      msgs = catd->info.m.msgs;
      data = catd->info.m.data;
      for (i = min(set_num, catd->set_nr) - 1 ; i >= 0 ; i--){
        if (set_num == sets[i].shdr_set_nr){
          first_msg = sets[i].shdr_msg;
  
          /*
           * Locate message in set
           */
          for (i = min(msg_num, sets[i].shdr_msg_nr) - 1 ; i >= 0 ; i--){
            if (msg_num == msgs[first_msg + i].msg_nr){
              i += first_msg;
              msg = data + msgs[i].msg_ptr;
              return msg;
            }
            if (msg_num > msgs[first_msg + i].msg_nr)
              return (char *)s;
          }
          return (char *)s;
        }
        if (set_num > sets[i].shdr_set_nr)
          return (char *)s;
      }
      return (char *)s;
  
  case MKMSGS :
      /*
       * get it from
       * a mkmsgs catalog
       */
      if (set_num > catd->set_nr)
        return (char *)s;
      set = &(catd->info.g.sets->sn[set_num -1]);
      if (msg_num > set->last_msg)
        return (char *)s;
      message_no = set->first_msg + msg_num -1;

      /*  Extract the message from the second file  */
      msg_ptr = *((int *)(catd->info.g.msgs + (message_no * sizeof(int))));
      p = (char *)(catd->info.g.msgs + msg_ptr);

      if (strcmp(p,DFLT_MSG) && strcmp(p,"Message not found!!\n"))
        return p;
      return (char *)s;
  
  default :
      return (char *)s;

  }
}
