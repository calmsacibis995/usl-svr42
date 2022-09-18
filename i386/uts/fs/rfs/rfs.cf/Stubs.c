/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/rfs/rfs.cf/Stubs.c	1.4"
#ident	"$Header: $"

#include	<sys/errno.h>

int rf_state;

rfubyte() { return(0); }
rfuword() { return(0); }
rsubyte() { return(0); }
rsuword() { return(0); }
rcopyin() { return(0); }
rcopyout() { return(0); }
rf_vp_preSVR4() { return(0); }
rf_clock() { return(0); }
rf_stime() { return(0); }
rfsys() { return(ENOPKG); }
rfc_disable_msg() { return(0); }
vtord() { return(0); }
duustat() { return(0); }
rf_ustat() { return(0); }
void rfc_inval() {}
