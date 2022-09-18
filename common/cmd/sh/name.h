/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/name.h	1.7.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/name.h,v 1.1 91/02/28 20:08:51 ccs Exp $"
/*
 *	UNIX shell
 */


#define	N_ENVCHG 0020
#define N_RDONLY 0010
#define N_EXPORT 0004
#define N_ENVNAM 0002
#define N_FUNCTN 0001

#define N_DEFAULT 0

struct namnod
{
	struct namnod	*namlft;
	struct namnod	*namrgt;
	unsigned char	*namid;
	unsigned char	*namval;
	unsigned char	*namenv;
	int	namflg;
};

