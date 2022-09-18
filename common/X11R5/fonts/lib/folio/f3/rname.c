/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/rname.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)rname.c 1.3 89/04/13";
#endif
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include    	<setjmp.h>

#include        "cdefs.h"
#include	"frmath.h"
#include    	"glbvars.h"
#include    	"rname.h"

/*  	----------------------	    */
char	*f3_RegisterName(name)
char	*name;
/*  	----------------------	    */
{
    struct namelist {
	struct namelist *next;
	char        name[1];
    };
    static struct namelist *nmroot;
    register struct namelist *p;
    for (p = nmroot; p; p = p->next)
	if (strcmp(p->name, name) == 0)
	    return p->name;
    p = (struct namelist *) malloc(strlen(name) + sizeof (struct namelist));
    strcpy(p->name, name);
    p->next = nmroot;
    nmroot = p;
    return p->name;
}
