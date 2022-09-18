/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/registers.h	1.1"
#ident	"$Header: registers.h 1.1 91/07/09 $"

CONST char * CONST regname[NGREG] = {
	"gs", "fs", "es", "ds", "edi", "ebp", "esp", "ebx",
	"edx", "ecx", "eax", "trapno", "err", "eip", "cs", "efl"
	"uesp", "ss"
};
