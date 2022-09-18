/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/devtab.h	1.1.15.2"
#ident	"$Header: devtab.h 1.4 91/06/25 $"

#ifndef _SYS_TIME_H
#include <sys/time.h>
#endif	/* _SYS_TIME_H */

#ifndef _SYS_MAC_H
#include <sys/mac.h>
#endif	/* _SYS_MAC_H */
/*
 * devtab.h
 *
 *	The definitions here are used by the functions defined
 *	to access/update the Device Database files and the
 *	device-group tables.
 */

/*
 *  Constant definitions
 *	NULL		Manifest constant NULL (null-address)
 *	TRUE		Boolean TRUE value
 *	FALSE		Boolean FALSE value
 *	SUCCESS		Integer value 0
 *	FAILURE		Integer value -1
 *	DDB_BUFSIZ	Initial buffersize for reading device database records
 *	DDB_BUFINC	Amount to increase device database record buffer
 *	DGRP_BUFSIZ	Initial buffersize for reading devgrp table records
 *	DGRP_BUFINC	Amount to increase device-group table record buffer
 *	XTND_MAXCNT	Maximum extend count (may have insane tables)
 *	DDB_ESCS	Characters that are escaped in fields in the devtab
 */

#ifndef	NULL
#define	NULL	(0)
#endif

#ifndef	TRUE
#define	TRUE	(1)
#endif

#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef SUCCESS
#define SUCCESS	0
#endif

#ifndef FAILURE
#define	FAILURE	-1
#endif

#define	DDB_BUFSIZ	512
#define	DDB_BUFINC	512
#define	DGRP_BUFSIZ	512
#define	DGRP_BUFINC	512
#define	XTND_MAXCNT	1

#define	DDB_ESCS	":\\\"\n"

#define MAXDDBFILES	3
#define MAXDDBPATH	40
#define MAXDSFLEN	512
#define MAXLVLSZ	20
#define MAXIDSZ		6

#define	DEV_ADD		1		/* add new attr-values         */
#define	DEV_MOD		2		/* modify existing attr-values */
#define	DEV_MODSEC	3		/* modify secdev + attr-values */
#define	DEV_REM		4		/* remove existing attr-values */

/*
 * Device Attribute Types
 *      - security attributes
 *      - tab (oam) attributes
 *      - device special file attributes
 */

/*
 * defines device attribute types
 */
#define TYPE_UNK	0
#define TYPE_SEC	1
#define TYPE_DSF	2
#define TYPE_TAB	4
/*
 * defines number of fixed attribute fields for each attr type
 */
#define MAXSECATTRS	12
#define MAXDSFATTRS	4
#define MAXTABATTRS	3

/*
 * macros that initialize entries with NULL field values:
 *	INIT_TABENTRY()
 *	INIT_SECENTRY()
 *	INIT_DSFENTRY()
 */
#define INIT_DSFENTRY(dsf) {\
			dsf->cdevice=(char *)NULL;\
			dsf->bdevice=(char *)NULL;\
			dsf->cdevlist=(char *)NULL;\
			dsf->bdevlist=(char *)NULL;\
			}
#define INIT_TABENTRY(tab) {\
			tab->cdevice=(char *)NULL;\
			tab->bdevice=(char *)NULL;\
			tab->pathname=(char *)NULL;\
			tab->secdev=(char *)NULL;\
			tab->comment=NULL;\
			tab->attrlist=(attr_val *)NULL;\
			} 
#define INIT_SECENTRY(sec) {\
			sec->range=(char *)NULL;\
			sec->state=(char *)NULL;\
			sec->mode=(char *)NULL;\
			sec->startup=(char *)NULL;\
			sec->st_level=(char *)NULL;\
			sec->st_owner=(char *)NULL;\
			sec->st_group=(char *)NULL;\
			sec->st_other=(char *)NULL;\
			sec->ual_enable=(char *)NULL;\
			sec->users=(char *)NULL;\
			sec->other=(char *)NULL;\
			}
/* Version of device.tab */
#define __4ES__	1 
#define __4dot0__ 2

/* matches the dsf_attrs[] array item */
#define CDEVICE 0 	
#define BDEVICE 1
/*
 *  Structure definitions for Device Database records:
 *	tab_entry	Describes an entry in the DDB_TAB file
 *	dsf_entry	Describes an entry in the DDB_DSFMAP file
 *	sec_entry	Describes an entry in the DDB_SEC file
 *	attr_val	Describes an attribute/value pair
 */

typedef struct at_val {
		char	*attr;		/* name of attribute          */
		char	*val;		/* value of attribute         */
		struct at_val *next;	/* ptr to next attr-value pair*/
	} attr_val;

typedef struct {
		char	*alias;		/* alias name of logical dev. */
		char	*cdevice;	/* path to character device file */
		char	*bdevice;	/* path to block device file */
		char	*pathname;	/* pathname attribute         */
		char	*secdev;	/* alias name of secure dev.  */
		int	comment;	/* TRUE if entry = comment    */
		attr_val *attrlist; 	/* attribute-value pairs      */
	} tab_entry;

typedef struct {
		char	*cdevice;	/* ptr to char dsf                    */
		char	*bdevice;	/* ptr to block dsf                   */
		char	*cdevlist;	/* ptr to a string of char dsfs       */
		char	*bdevlist;	/* ptr to a string of block dsfs      */
	} dsf_entry;

typedef struct {
		char	*alias;		/* secure alias name of device    */
		char	*range;		/* range= hilevel,lolevel         */
		char	*state;		/* state= private/public /privpub */
		char	*mode;		/* mode= static/dynamic           */
		char	*startup;	/* startup= yes/no                */
		char	*st_level;	/* startup_level= lid             */
		char	*st_owner;	/* startup_owner= uid>rwx         */
		char	*st_group;	/* startup_group=gid>rwx          */
		char	*st_other;	/* startup_other=>rwx             */
		char	*ual_enable;	/* User Authorization enabled= y/n*/
		char	*users;		/* users= uid1>y,uid2>n...        */
		char	*other;		/* other authorization= >y/n      */
	} sec_entry;

typedef struct {
		tab_entry	*tab;		/* oam attributes    */
		dsf_entry	*dsf;		/* dev special files */
		sec_entry	*sec;		/* sec attributes    */
	} dev_record;

typedef enum {
		CDEV=1,		/* cdevice  */
		BDEV,		/* bdevice  */
		CDEVL,		/* cdevlist */
		BDEVL		/* bdevlist */
	} dsf_t;

typedef struct {
		char		*dsf;
		dsf_t		type;
	} dev_file;

typedef struct {
		char		user[6];	/* ASCII uid           */
		char		perm[2];	/* permission >y or >n */
	} ual_entry;

typedef struct {
		char 		*range;
		char		*state;
		char 		*mode;
	} essentials;

/*
 * DDB Error related definitions:
 *	msgbuf	error message buffer	
 */
#define	SEV_NONE	0
#define	SEV_ERROR	1
#define SEV_WARN	2

#define EX_INTPRB	1	/* Internal problems: no memory */
#define EX_USAGE	1	/* Usage message (for putdev)	*/
#define EX_ACCESS	2	/* Problems creating new DDB files 
				 * or reading old DDB files	*/
#define EX_CONSTY	2	/* DDB files are in inconsisten state */
#define EX_ENODEV	3	/* device not defined in the DDB */
#define EX_INVAL	4	/* invalid value for b/cdevice, b/cdevlist, pathname */
#define EX_NOATTR	4	/* attribute being removed not defined*/

#define EX_NOPKG	5	/* ES package not installed */
#define EX_ERROR	6	/* Rest of the errors */

#define ACT_QUIT	1
#define ACT_CONT	2

#define WARNING		999	/* Used as a flag when the error buffer
				 * has been set to display a warning */

#define NOATTR		9999	/* Value returned from mod_tabent, if the
				 * attribute requested for removal was not 
				 * defined */

#define ERRMAX		1024

#define PUTDEV		"putdev"
#define DDBCONV		"ddbconv"

extern char Cmdname[]; 		/* Used by libadm's functions that need to
				 * invoke err_report. Defined in putdev.c */

extern int __tabversion__;	/* Represents the type of format found
				 * in the device.tab, 4.0 or 4ES 
				 */

struct msgbuf {
	int	sev;
	int	excode;
	int	errnum;
	char	text[ERRMAX];
};

/*
 * Error messages for PUTDEV command
 */
#define	E_USAGE	"incorrect usage\n\
USAGE: putdev -a alias [secdev=alias] [attribute=value [...]]\n\
       putdev -m device attribute=value [attribute=value [...]]\n\
       putdev -d device [attribute[...]]\n\
       putdev -p device attribute=value[,value...]\n\
       putdev -r device attribute=value[,value...]\n"

#define E_CONSTY	"Device Database in inconsistent state - notify administrator\n"

#define E_LTDB		"LTDB is not accessible - notify administrator\n"

#define E_NOMEM		"insufficient memory\n"

#define E_ACCESS	"Device Database could not be accessed or created\n"

#define E_AEXIST	"%s already exists in Device Database\n"

#define E_ANOTDEF	"\"%s\" not defined in Device Database\n"

#define E_NODEV		"%s does not exist in Device Database\n"

#define E_MULTDEF	"\"%s\" multiply defined for attribute \"%s\"\n"

#define E_DSFEXIST	"\"%s\" already exists in Device Database\n"

#define E_DINVAL	"invalid alias or invalid pathname \"%s\"\n"

#define E_LVLDOM	"hilevel does not dominate lolevel in attribute range\n"

#define E_INVAL		"invalid value for attribute \"%s\"\n"

#define E_INLVL		"invalid value for attribute \"%s\"\n \
	level= \"%s\" not defined in LTDB\n"

#define E_INUID		"invalid value for attribute \"%s\"\n \
	user/uid= \"%s\" not defined in system\n"

#define E_INGID		"invalid value for attribute \"%s\"\n \
	group/gid= \"%s\" not defined in system\n"

#define E_INPRM		"invalid value for attribute \"%s\"\n \
	invalid permissions specified \"%s\"\n"

#define E_INDLM		"invalid value for attribute \"%s\"\n \
	invalid delimiter specified in \"%s\"\n"

#define E_ESSSEC	"\"%s\" does not define essential security attributes(range, mode, state)\n"

#define E_SECENT	"\"%s\" must be defined with essential security attributes(range, mode, state)\n"

#define E_NOATTR	"\"%s\" not defined for %s\n"

#define E_NOTSEC	"cannot specify security attrs for alias and map to another secdev \"%s\"\n"

#define E_DDBUSE	"Device Database in use - Try again later.\n"

#define E_NOPKG		"invalid attribute\n \
UX:%s:ERROR: system service not installed\n"

#define E_ESSTOG	"essential security attributes (range, mode, state) must be specified together for \"%s\"\n"


/*
 * Error messages for admalloc command 
 */
#define E_PERM		"permission denied for %s\n"
#define E_NOPATH	"\"%s\" is undefined or inaccessible for %s\n"
#define E_NOTDSF	"\"%s\" is not a block or character special file for %s\n"
#define E_ROFS		"\"%s\" resides on a read-only filesystem for %s\n"
#define E_NOLVL		"\"%s\" resides on a filesystem that does not support per-file labelling for %s\n"
#define E_NOACL		"\"%s\" resides on a filesystem that does not support access control lists for %s\n"


/*
 *  Structure definitions for device-group records:
 *	struct dgrptabent	Describes a record in the device-group table
 *	struct member		Describes a member of a device group
 */

/*
 *  struct dgrptabent
 *	entryno			The entry number of this record 
 *	comment			Comment flag, TRUE if record is a comment
 *	name			The name of the device group
 *	memberspace		The buffer containing the members of the
 *				device group
 *	membership		Pointer to the head of the list of
 *				members in the group.
 */

struct dgrptabent {
	int		entryno;	/* Entry number of this record */
	int		comment;	/* TRUE if a comment record */
	char	       *name;		/* Device group name */
	char	       *dataspace;	/* Buffer containing membership */
	struct member  *membership;	/* Ptr to top of membership list */
};


/*
 *  struct member
 *	name			Member name (a device alias or pathname)
 *	next			Ptr to next item in the list
 */

struct member {
	char	       *name;		/* Member name */
	struct member  *next;		/* Next member in the list */
};



/*
 *  Global function and data definitions:
 *
 *	_setdgrptab()		Rewind the open device-group table
 *	_enddgrptab()		Close the open device table
 *	_getdgrptabent()	Get the next device-group table entry
 *	_freedgrptabent()	Frees space alloced to a dev-grp table entry
 *	_getdgrprec()		Gets a specific device-group table entry
 *	_opendgrptab()		Open the device group table
 *	_dgrptabpath()		Get the pathname of the device group table file
 *
 */

	void			_setdgrptab();
	void			_enddgrptab();
	struct dgrptabent      *_getdgrptabent();
	void			_freedgrptabent();
	struct dgrptabent      *_getdgrprec();
	int			_opendgrptab();
	char		       *_dgrptabpath();

/*
 * Device Database functions:
 */
#if defined(__STDC__)
extern int	lock_ddb();
extern int	unlock_ddb();
extern FILE	*opentmpddb(char *, char **);
extern int	rmtmpddb(char *);
extern int	getaliasmap(char *, int *, int *, char *);
extern tab_entry *getalias(char *,int *);
extern int	_opendevtab(char *);
extern void	_enddevtab();
extern char	 *_devtabpath();
extern int	getdsfmap(char *, int *, char *, char *);
extern int	getnextdsf(FILE *, char *, int, char *, int *);
extern int	make_devrec(char *, char **, int, dev_record *);
extern sec_entry *get_secent(char *);
extern dsf_entry *get_dsfent(char *);
extern tab_entry *get_tabent(char *);
extern int	put_secent(sec_entry *);
extern int	put_dsfent(dsf_entry *, char *, char *);
extern int	put_tabent(tab_entry *);
extern int	rem_secent(char *);
extern int	rem_dsfent(char *);
extern int	rem_tabent(char *);
extern int	valid_alias(char *);
extern int	valid_path(char *);
extern int	parse_range(char *, level_t *, level_t *);
extern int	parse_state(char *);
extern int	parse_mode(char *);
extern int	parse_level(char *, level_t *);
extern char 	*getfield(char *, char *, char **);
extern char	*skpfield(char *, char *);
extern char	*getquoted(char *, char **);
extern char	*read_ddbrec(FILE *);
extern int	write_ddbrec(FILE *, char *);
extern int	ddb_errget();
#else
extern int	lock_ddb();
extern int	unlock_ddb();
extern FILE	*opentmpddb();
extern int	rmtmpddb();
extern int	getaliasmap()
extern tab_entry *getalias();
extern int	_opendevtab();
extern void	_enddevtab();
extern char	 *_devtabpath();
extern int	getdsfmap();
extern int	getnextdsf();
extern int	make_devrec();
extern sec_entry *get_secent();
extern dsf_entry *get_dsfent();
extern tab_entry *get_tabent();
extern int	put_secent();
extern int	put_dsfent();
extern int	put_tabent();
extern int	rem_secent();
extern int	rem_dsfent();
extern int	rem_tabent();
extern int	valid_alias();
extern int	valid_path();
extern int	parse_range();
extern int	parse_state();
extern int	parse_mode();
extern int	parse_level();
extern char 	*getfield();
extern char	*skpfield();
extern char	*getquoted();
extern char	*read_ddbrec();
extern int	write_ddbrec();
extern int	ddb_errget();
#endif /* __STDC__ */
