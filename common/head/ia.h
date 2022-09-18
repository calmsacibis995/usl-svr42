/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:ia.h	1.4.5.4"
#ident  "$Header: ia.h 1.3 91/06/21 $"

#define AUDITMASK	"/etc/security/ia/audit"
#define OAUDITMASK	"/etc/security/ia/oaudit"
#define MASKTEMP 	"/etc/security/ia/masktmp"
#define INDEX		"/etc/security/ia/index"
#define OINDEX		"/etc/security/ia/oindex"
#define TMPINDEX	"/etc/security/ia/indextmp"
#define MASTER		"/etc/security/ia/master"
#define OMASTER		"/etc/security/ia/omaster"
#define MASTMP		"/etc/security/ia/mastmp"
#define	LVLDIR		"/etc/security/ia/level/"
#define	OLVLDIR		"/etc/security/ia/level/o:"

#define	IA_READ		1
#define	IA_WRITE	2
#define	IA_FREE		3


/* user audit structure for I&A */
struct	adtuser {
	char	ia_name[32];
	ulong_t	ia_amask[ADT_EMASKSIZE];
};

/* structure for the index file */
struct	index {
	char	name[32];
	long	offset;
	long	length;
};

/* structure for the master */
struct	master {
	char	ia_name[32];
	char    ia_pwdp[13] ; /* user password */
	long	ia_uid;
	long	ia_gid;
	long ia_lstchg ; /* password lastchanged date */
	long ia_min ; /* minimum number of days between password changes */
	long ia_max ; /* number of days password is valid */
	long ia_warn ; /* number of days to warn user to change passwd */
	long ia_inact ; /* number of days the login may be inactive */
	long ia_expire ; /* date when the login is no longer valid */
	long ia_flag ; /* not used  */
	ulong_t	ia_amask[ADT_EMASKSIZE];
	long	ia_dirsz;
	long	ia_shsz;
	long	ia_lvlcnt;
	long	ia_sgidcnt;
	char	*ia_dirp;
	char	*ia_shellp;
	level_t	*ia_lvlp;
	gid_t	*ia_sgidp;
};

typedef struct master *uinfo_t;

#if defined(__STDC__)

#ifndef _STDIO_H
#include <stdio.h>
#endif

/*************************************************
	 Declare all ANSI-C I&A functions 
*************************************************/
extern void	setadtent(void), endadtent(void);
extern void	cnvxmask(unsigned long *out, char *in);
extern	int	getadtent(struct adtuser *), fgetadtent(FILE *, struct adtuser *); 
extern	int	getadtnam(char *, struct adtuser *);
extern	int	putadtent(struct adtuser *, FILE *), lckadtmskf(void), ulckadtmskf(void);
extern	int	getiasz(struct index *), getianam(struct index *, struct master *);
extern	int	putiaent(char *, struct master *);
extern	int	lvlia(int, level_t **, char *, long *);
extern	int	ia_openinfo(char *, uinfo_t *);
extern	int	ia_get_sgid(uinfo_t, gid_t **, long *);
extern	int	ia_get_lvl(uinfo_t, level_t **, long *);
extern	void	ia_closeinfo(uinfo_t);
extern	void	ia_get_mask(uinfo_t, ulong_t * );
extern	void	ia_get_uid(uinfo_t, uid_t *);
extern	void	ia_get_gid(uinfo_t, gid_t *);
extern	void	ia_get_dir(uinfo_t, char **);
extern	void	ia_get_sh(uinfo_t, char **);
extern	void	ia_get_logpwd(uinfo_t, char **);
extern	void	ia_get_logchg(uinfo_t, long *);
extern	void	ia_get_logmin(uinfo_t, long *);
extern	void	ia_get_logmax(uinfo_t, long *);
extern	void	ia_get_logwarn(uinfo_t, long *);
extern	void	ia_get_loginact(uinfo_t, long *);
extern	void	ia_get_logexpire(uinfo_t, long *);
extern	void	ia_get_logflag(uinfo_t, long *);

#else

/*************************************************
	Declare all I&A functions   
*************************************************/
void 	setadtent(), endadtent(),cnvxmask();
int   	getadtent(), fgetadtent(), getadtnam() ;
int 	putadtent(), getiasz(), getianam();
int 	putiaent(), lckadtmskf(), ulckadtmskf();
int 	lvlia();
int	ia_openinfo();
int	ia_get_sgid();
int	ia_get_lvl();
void	ia_closeinfo();
void	ia_get_mask();
void	ia_get_uid();
void	ia_get_gid();
void	ia_get_dir();
void	ia_get_sh();
void	ia_get_logpwd();
void	ia_get_logchg();
void	ia_get_logmin();
void	ia_get_logmax();
void	ia_get_logwarn();
void	ia_get_loginact();
void	ia_get_logexpire();
void	ia_get_logflag();

#endif
