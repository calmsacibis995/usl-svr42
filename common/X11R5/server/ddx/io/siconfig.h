/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/io/siconfig.h	1.1"

/* (c) Copyright 1988 INTERACTIVE Systems Corporation */

#ifndef	SICONFIG_H
#define	SICONFIG_H

#include	"../si/sidep.h"

extern	void		config_setfile( /* filename */ );
extern	char		*config_getfile();
extern	int		config_setent();
extern	void		config_endent();
extern	SIConfigP	config_getent();
extern	SIConfigP	config_getresource();

#endif	/* SICONFIG_H */
