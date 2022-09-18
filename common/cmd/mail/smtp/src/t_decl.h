/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/t_decl.h	1.1.2.2"
#ident "@(#)t_decl.h	1.1 'attmail mail(1) command'"

#ifndef T_LISTEN
#include <tiuser.h>
#endif

extern int	t_accept proto((int, int, struct t_call *));
extern char *	t_alloc proto((int, int, int));
extern int	t_bind proto((int, struct t_bind *, struct t_bind *));
extern int	t_close proto((int));
extern int	t_connect proto((int, struct t_call *, struct t_call *));
extern int	t_free proto((char *, int));
extern int	t_listen proto((int, struct t_call *));
extern int	t_look proto((int));
extern int	t_open proto((char *, int, struct t_info *));
extern int	t_rcvdis proto((int, struct t_discon *));
