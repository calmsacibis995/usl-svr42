/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:error.h	1.4"
#endif

/*
** error.h - This file contains the declarations for `progname' and the
** functions defined in "error.c".
*/


#ifndef ERROR_H
#define ERROR_H


extern char	*progname;		/* If not NULL, all error messages */
					/* are prefixed by "<progname>: "; */
					/* usually set to `argv[0]' in */
					/* main() */


/*
** Print message to stderr using `template' as a printf control string and
** `value' as an optional string matching a "%s" in `template'.  The message
** is prefixed by "<progname>: " if `progname' is not NULL, and is followed
** by the `sys_errlist' entry corresponding to the current value of `errno'.
** exit(2) is called after the message is printed with `exit_status' as its
** argument.
*/
extern void error_exit();
/*
  char  *template, *value;
  int   exit_status;
*/


/*
** More general form of error_exit(); the message is printed to the stream
** `fp'.
*/
extern void error_fexit();
/*
  FILE  *fp;
  char  *template, *value;
  int   exit_status;
*/


/*
** Print message to stderr using `template' as a printf control string and
** `value' as an optional string matching a "%s" in `template'.  The message
** is prefixed by "<progname>: " if `progname' is not NULL, and is followed
** by the `sys_errlist' entry corresponding to the current value of `errno'.
** The number of characters printed is returned; a value of -1 indicates an
** error.
*/
extern int error_print();
/*
  char  *template, *value;
*/


/*
** More general form of error_print(); the message is printed to the stream
** `fp'.
*/
extern int error_fprint();
/*
  FILE  *fp;
  char  *template, *value;
*/


/*
** Returns entry from `sys_errlist' indexed by `n'.  If `n' is out of range,
** NULL is returned.
*/
extern char *error_string();
/*
  int   n;
*/


#endif	/* ERROR_H */
