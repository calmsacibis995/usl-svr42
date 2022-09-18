/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/printers/p_head.c	1.6.1.3"
#ident	"$Header: $"

#include "sys/types.h"

#include "lp.h"
#include "printers.h"

struct {
	char	*v;
	short	len,
		okremote;
}  prtrheadings [PR_MAX] = {

#define	ENTRY(X)	X, sizeof(X)-1

	ENTRY("Banner:"),		0,	/* PR_BAN */
	ENTRY("CPI:"),			0,	/* PR_CPI */
	ENTRY("Character sets:"),	1,	/* PR_CS */
	ENTRY("Content types:"),	1,	/* PR_ITYPES */
	ENTRY("Device:"),		0,	/* PR_DEV */
	ENTRY("Dial:"),			0,	/* PR_DIAL */
	ENTRY("Fault:"),		0,	/* PR_RECOV */
	ENTRY("Interface:"),		0,	/* PR_INTFC */
	ENTRY("LPI:"),			0,	/* PR_LPI */
	ENTRY("Length:"),		0,	/* PR_LEN */
	ENTRY("Login:"),		0,	/* PR_LOGIN */
	ENTRY("Printer type:"),		1,	/* PR_PTYPE */
	ENTRY("Remote:"),		1,	/* PR_REMOTE */
	ENTRY("Speed:"),		0,	/* PR_SPEED */
	ENTRY("Stty:"),			0,	/* PR_STTY */
	ENTRY("Width:"),		0,	/* PR_WIDTH */
#if	defined(CAN_DO_MODULES)
	ENTRY("Modules:"),		0,	/* PR_MODULES */
#endif
	ENTRY("Range:"),		1,	/*  PR_HILEVEL, PR_LOLEVEL  */

#undef	ENTRY

};
