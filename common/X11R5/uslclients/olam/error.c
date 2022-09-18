/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:error.c	1.3"
#endif

/*
** error.c - This file contains general purpose functions for dealing with
** error notification.
*/


#include <stdio.h>

#include "error.h"


char	*progname = NULL;		/* Program's name; usually set to */
					/* `argv[0]' in main() */


/*
** Internal routine to print the error string corresponding to the current
** value of `errno'.  `with_parens_p' controls whether the string is
** surrounded by parens.
** If `errno' indexes a valid string, the number of characters printed is
** returned, else 0 is returned.
*/
static int
_print_syserr(fp, with_parens_p)
  FILE	*fp;
  int	with_parens_p;
{
  extern int	errno;
  char		*sys_err;


  sys_err = error_string(errno);
  if (sys_err != NULL)			/* `errno' is in range */
    if (with_parens_p)			/* Use parens */
      return fprintf(fp, " (%s)", sys_err);
    else				/* No parens */
      return fputs(sys_err, fp);
  else					/* `errno' is out of range */
    return 0;

} /* _print_syserr() */


/*
** Print message to stderr using `template' as a printf control string and
** `value' as an optional string matching a "%s" in `template'.  The message
** is prefixed by "<progname>: " if `progname' is not NULL, and is followed
** by the `sys_errlist' entry corresponding to the current value of `errno'.
** exit(2) is called after the message is printed with `exit_status' as its
** argument.
*/
void
error_exit(template, value, exit_status)
  char	*template, *value;
  int	exit_status;
{

  error_fexit(stderr, template, value, exit_status);

} /* error_exit() */


/*
** More general form of error_exit(); the message is printed to the stream
** `fp'.
*/
void
error_fexit(fp, template, value, exit_status)
  FILE	*fp;
  char	*template, *value;
  int	exit_status;
{
  extern void	exit();


  (void)error_fprint(fp, template, value);
  exit(exit_status);

} /* error_fexit() */


/*
** Print message to stderr using `template' as a printf control string and
** `value' as an optional string matching a "%s" in `template'.  The message
** is prefixed by "<progname>: " if `progname' is not NULL, and is followed
** by the `sys_errlist' entry corresponding to the current value of `errno'.
** The number of characters printed is returned; a value of -1 indicates an
** error.
*/
int
error_print(template, value)
  char	*template, *value;
{

  return error_fprint(stderr, template, value);

} /* error_print() */


/*
** More general form of error_print(); the message is printed to the stream
** `fp'.
*/
int
error_fprint(fp, template, value)
  FILE	*fp;
  char	*template, *value;
{
  static int	_print_syserr();
  int		n;			/* Return value of various printing */
					/* routines */
  int		nchars;			/* Number of characters printed */


  if (progname != NULL)			/* `progname' has been set */
    if ((nchars = fprintf(fp, "%s: ", progname)) < 0)
					/* Print "<progname>: " */
      return -1;			/* Error in fprintf(3) */

  if (template != NULL)			/* A template was provided */
    {
      if ((n = fprintf(fp, template, value)) < 0)
					/* Print message */
	return -1;			/* Error in fprintf(3) */
      else
	nchars += n;

      if ((n = _print_syserr(fp, 1)) < 0)
					/* Print system error string */
	return -1;			/* Error in _print_syserr() */
      else
	nchars += n;
    }
  else					/* No template; just print system */
					/* error string */
    if (( n = _print_syserr(fp, 0)) < 0)
					/* Print system error string without */
					/* parens  */
      return -1;			/* Error in _print_syserr() */
    else
      nchars += n;

  if (putc('\n', fp) == EOF)		/* Print final newline */
    return -1;				/* Error in putc(3) */
  else
    nchars += 1;

  return nchars;			/* Return number of printed chars. */

} /* error_fprint() */


/*
** Returns entry from `sys_errlist' indexed by `n'.  If `n' is out of range,
** NULL is returned.
*/
char *
error_string(n)
  int	n;
{
  extern char	*sys_errlist[];
  extern int	sys_nerr;


  if (n > 0 && n < sys_nerr)		/* In range */
    return sys_errlist[ n ];
  else					/* Out of range */
    return NULL;

} /* error_string() */
