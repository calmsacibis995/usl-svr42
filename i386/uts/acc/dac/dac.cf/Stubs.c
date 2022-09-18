/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:acc/dac/dac.cf/Stubs.c	1.3"
#ident	"$Header: $"

int dac_installed = 0;

int aclipc() { return nopkg(); }
int ipcaclck() { return noreach(); }
int acl() { return nopkg(); }
int acl_getmax() { return 0; }
int acl_valid() { return nopkg(); }
