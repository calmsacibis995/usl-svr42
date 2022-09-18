/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/vt0/open.c	1.1"
#ident	"$Header: $"

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

int xnow;
int ynow;
float boty = 0.;
float botx = 0.;
float oboty = 0.;
float obotx = 0.;
float scalex = 1.;
float scaley = 1.;
int vti = -1;

openvt ()
{
		vti = open("/dev/vt0",1);
		return;
}
openpl()
{
	vti = open("/dev/vt0",1);
	return;
}
