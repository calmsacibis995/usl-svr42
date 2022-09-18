/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:assert.h	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989,  Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)assert.h	2.1	90/04/25
 *
 */

#ifdef DEBUG
#define assert(ex)	{if (!(ex)){(void)fprintf(stderr,"Assertion \"ex\" failed: file \"%s\", line %d\n", __FILE__, __LINE__);abort();}}
#else
#define assert(ex)
#endif
