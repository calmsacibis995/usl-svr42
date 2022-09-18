/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/global.c	1.6.2.3"
#ident  "$Header: global.c 1.2 91/06/26 $"

#include "global.h"
#include <dial.h>
#include <netconfig.h>
#include <netdir.h>

char	Scratch[MSGSIZE];	/* general scratch buffer */
char	msg[MSGSIZE];		/* general debugging message buffer */
CALL	Call;
CALL	*Callp=&Call;
struct	nd_hostserv	Nd_hostserv;
int	Debugging=0;
int	netfd;		/* fd into the network */
int	returnfd;	/* authenticated fd to return */
int	Pid;		/* pid of the dial request client	*/
