/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ifndef _FS_RMNTTAB_H	/* wrapper symbol for kernel use */
#define _FS_RMNTTAB_H	/* subject to change without notice */

#ident	"@(#)rmount:rmnttab.h	1.1.1.2"
#ident	"@(#)rmount:rmnttab.h	1.1.1.2"

#define	RMNTTAB	"/etc/rfs/rmnttab"
#define	RMNT_LINE_MAX	1024

#define	RMNT_TOOLONG	1	/* entry exceeds RMNT_LINE_MAX */
#define	RMNT_TOOMANY	2	/* too many fields in line */
#define	RMNT_TOOFEW	3	/* too few fields in line */

#define	rmntnull(rmp)\
	((rmp)->rmnt_special	= \
	 (rmp)->rmnt_mountp	= \
	 (rmp)->rmnt_fstype	= \
	 (rmp)->rmnt_rmntopts	= \
	 (rmp)->rmnt_time	= \
	 (rmp)->rmnt_lvl	= NULL)

#define	putrmntent(fp, rmp)\
	(void)fprintf((fp), "%s\t%s\t%s\t%s\t%s\t%s\n",\
		(rmp)->rmnt_special	? (rmp)->rmnt_special	: "-",\
		(rmp)->rmnt_mountp	? (rmp)->rmnt_mountp	: "-",\
		(rmp)->rmnt_fstype	? (rmp)->rmnt_fstype	: "-",\
		(rmp)->rmnt_mntopts	? (rmp)->rmnt_mntopts	: "-",\
		(rmp)->rmnt_time	? (rmp)->rmnt_time	: "-",\
		(rmp)->rmnt_lvl		? (rmp)->rmnt_lvl	: "-")

struct rmnttab {
	char	*rmnt_special;
	char	*rmnt_mountp;
	char	*rmnt_fstype;
	char	*rmnt_mntopts;
	char	*rmnt_time;
	char	*rmnt_lvl;
};

#ifdef __STDC__
extern int	getrmntent(FILE *, struct rmnttab *);
#else
extern int	getrmntent( );
#endif

#endif /* _FS_RMNTTAB_H */
