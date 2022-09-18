/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/resource.h	1.5"
#endif

#ifndef _RESOURCE_H
#define _RESOURCE_H

typedef struct {
	char *			name;
	char *			value;
} Resource;

extern int			read_buffer();
extern int			write_buffer();
extern void			resources_to_buffer();
extern void			buffer_to_resources();
extern void			merge_resources();
extern void			delete_resources();
extern void			free_resources();
extern void			delete_RESOURCE_MANAGER();
extern void			change_RESOURCE_MANAGER();
extern void			programs_to_buffer();
extern void			buffer_to_programs();
extern char *			resource_value();

#endif
