/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/table.h	1.2"
#ifdef H_SCCSIDS
#include <sccs.h>
SCCSID(@(#)table.h	6.1	LCC);	/* Modified: 14:55:40 10/15/90 */
#endif 

/* table.h - common info required for character translation tables */
#ifndef H_TABLE
#define H_TABLE

#include <lcs.h>

#ifndef DEFAULT_CHAR
#define DEFAULT_CHAR '*'
#endif

#ifndef NO_EXTERNS
extern short country;
extern char  default_char;

extern char dos_table_name[];
extern char unix_table_name[];

extern lcs_tbl unix_table, dos_table;		/* pointers to the UNIX
						 * and DOS translation
						 * tables
						 */
extern int  cur_table_flag;
#endif /* NO_EXTERNS */

#endif /* H_TABLE */
