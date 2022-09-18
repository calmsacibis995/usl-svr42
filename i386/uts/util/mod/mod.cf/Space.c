/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/mod.cf/Space.c	1.10"
#ident	"$Header: $"

#include	<config.h>
#include	<sys/pic.h>
#include	<sys/mod_k.h>
#include	<sys/mod_intr.h>

#define	MOD_MAX_INTR	NPIC * 8

struct	mod_shr_v	*mod_shr_ivect[MOD_MAX_INTR];
char	mod_iv_locks[MOD_MAX_INTR];

struct	modctl	*mod_shadowcsw[CDEVSWSZ];
struct	modctl	*mod_shadowbsw[BDEVSWSZ];
struct	modctl	*mod_shadowfsw[FMODSWSZ];
struct	modctl	*mod_shadowvfssw[VFSSWSZ];
