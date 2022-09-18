/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:include/users.h	1.12.2.3"
#ident  "$Header: users.h 1.2 91/06/27 $"

#if	!defined(_LP_USERS_H)
#define	_LP_USERS_H

#include "stdio.h"

typedef struct
{
    short	priority_limit;
}
USER;

#if	defined(__STDC__)

int		putuser ( char * , USER * );
int		deluser ( char * );
int		getdfltpri ( void );
void	trashusers ( void );

USER *		getuser ( char *);

#else

int		putuser(),
		deluser(),
		getdfltpri();
void		trashusers();

USER *		getuser();

#endif

#define LEVEL_DFLT 20
#define LIMIT_DFLT 0

#define TRUE  1
#define FALSE 0

#define PRI_MAX 39
#define	PRI_MIN	 0

#define LPU_MODE 0664

struct user_priority
{
    short	deflt;		/* priority to use when not specified */
    short	deflt_limit;	/* priority limit for users not
				   otherwise specified */
    char	**users[PRI_MAX - PRI_MIN + 1];
};

#endif
#if		defined(__STDC__)
int		add_user(struct user_priority *, char *, int);
int		del_user(struct user_priority *, char *);
void		output_tbl(FILE *, struct user_priority *);
#else
int		add_user();
int		del_user();
void		output_tbl();
#endif
