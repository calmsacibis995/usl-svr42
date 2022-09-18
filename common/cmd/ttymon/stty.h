/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/stty.h	1.3.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ttymon/stty.h,v 1.1 91/02/28 20:16:02 ccs Exp $"

#define ASYNC	1
#define FLOW	2
#define WINDOW	4
#define TERMIOS 8

struct	speeds {
	const char	*string;
	int	speed;
};

struct mds {
	const char	*string;
	long	set;
	long	reset;
};

