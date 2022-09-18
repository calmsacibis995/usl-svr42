/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:include/systems.h	1.7.1.4"
#ident "$Header: 1.1 91/02/28 18:04:31 $"

# define	SYS_PASSWD	0
# define	SYS_PROTO	1
# define	SYS_TMO		2
# define	SYS_RETRY	3
# define	SYS_COMMENT	4
# define	SYS_MAX		5

/**
 ** The internal copy of a system as seen by the rest of the world:
 **/

typedef struct SYSTEM
{
    char	*name;		/* name of system (redundant) */
    char	*passwd;        /* the encrypted passwd of the system */
    char	*reserved1;
    int		protocol;	/* lp networking protocol s5|bsd */
    char	*reserved2;	/* system address on provider */
    int		timeout;	/* maximum permitted idle time */
    int		retry;		/* minutes before trying failed conn */
    char	*reserved3;
    char	*reserved4;
    char	*comment;
} SYSTEM;

# define	NAME_S5PROTO	"s5"
# define	NAME_BSDPROTO	"bsd"
# define	NAME_NUCPROTO	"nuc"

# define	S5_PROTO	1
# define	BSD_PROTO	2
# define	NUC_PROTO	3

/**
 ** Various routines.
 **/

#if	defined(__STDC__)

SYSTEM		*getsystem ( const char * );

int		putsystem ( const char *, const SYSTEM * ),
		delsystem ( const char * );
void		freesystem( SYSTEM * );

#else

SYSTEM		*getsystem();

int		putsystem(),
		delsystem(),
		freesystem();

#endif
