/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/varshrink.c	1.4.3.3"

#include	<stdio.h>
#include 	"terror.h"	/* dmd s15 */
#include	"wish.h"
#include	"var_arrays.h"

/*
 * shrink the actual space used by a v_array as much as possible
 * note that this requires the process to allocate more space
 * before giving some back, so it may actually INCREASE the data
 * segment size of the process.  If used, array_shrink should be
 * called before adding things to other v_arrays, since perhaps
 * one of them can take advantage of the freed space.
 */
struct v_array *
array_shrink(array)
struct v_array	array[];
{
	register struct v_array	*ptr;
	register struct v_array	*newptr;

	ptr = v_header(array);
	if ((newptr = (struct v_array *)realloc(ptr, sizeof(struct v_array) + ptr->tot_used * ptr->ele_size)) == NULL)
		fatal(NOMEM, nil);	/* dmd s15 */
	return v_body(newptr);	/* chged ptr to newptr. abs k14.2 */
}
