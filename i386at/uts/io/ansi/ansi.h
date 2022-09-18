/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_ANSI_ANSI_H	/* wrapper symbol for kernel use */
#define _IO_ANSI_ANSI_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/ansi/ansi.h	1.8"
#ident	"$Header: $"

/*
 * Definitions for Integrated Workstation Environment ANSI x3.64 
 * terminal control language parser.
 */

#define ANSI_MAXPARAMS	5	/* maximum number of ANSI paramters */
#define ANSI_MAXTAB	40	/* maximum number of tab stops */
#define ANSI_MAXFKEY	30	/* max length of function key with <ESC>Q */

#define	ANSIPSZ		64	/* max packet size sent by ANSI */

/*
 * Font values for ansistate.
 */
#define	ANSI_FONT0	0	/* primary font (default) */
#define	ANSI_FONT1	1	/* first alternate font */
#define	ANSI_FONT2	2	/* second alternate font */

#define	ANSI_BLKOUT	0x8000	/* scroll lock, for M_START, M_STOP */

struct ansi_state {		/* state for ansi x3.64 emulator */
	unsigned short	a_flags;	/* flags for this x3.64 terminal */
	unsigned char	a_font;		/* font type */
	unsigned char	a_state;	/* state in output esc seq processing */
	unsigned char	a_gotparam;	/* does output esc seq have a param */
	unsigned short	a_curparam;	/* current param # of output esc seq */
	unsigned short	a_paramval;	/* value of current param */
	short a_params[ANSI_MAXPARAMS]; /* parameters of output esc seq */
	char a_fkey[ANSI_MAXFKEY];	/* work space for function key */
	struct msgb	*a_wmsg;	/* ptr to data msg being assembled */
	struct queue	*a_wqp;		/* ptr to write q for assoc. stream */
	struct queue	*a_rqp;		/* ptr to read q for assoc. stream */
};

typedef struct ansi_state ansistat_t;

#endif /* _IO_ANSI_ANSI_H */
