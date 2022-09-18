/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/inst.h	1.8"
#ident	"$Header:"

/* Header file for Installable Drivers commands */

/* Nested includes to resolve FILE and DIR */
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#define OMAXMIN		0xff	/* from util/mkdev.h */
#define	MAXOBJNAME	128	/* from mem/bootconf.h */

/* macro to check if a given char is in a given string */
#define	INSTRING(str, c)	(strchr(str, c) != NULL)
/* string equality macro */
#define equal(s1, s2)		(strcmp(s1, s2) == 0)

/* path names of ID directories */
#define ROOT	"/etc/conf"		/* root of ID */
#define EROOT	"/etc"			/* directory for installing environment */
#define DROOT	"/dev"			/* directory for installing nodes */
#define	CFDIR	"cf.d"			/* Master & System files for building */
#define	PKDIR	"pack.d"
#define	CFPATH	"/etc/conf/cf.d"

/* Special "names" for getinst() */
#define FIRST	"0"
#define	NEXT	"1"
#define RESET	"2"

/* ID file type codes */
#define	MDEV   	0		/* Master device file */
#define MDEV_D	1		/* Files in mdevice.d directory */
#define	SDEV   	2		/* System device file */
#define SDEV_D	3		/* Files in sdevice.d directory */
#define	MTUN	4		/* Master tunable parameter file */
#define MTUN_D	5		/* Files in mtune.d directory */
#define	STUN	6		/* System tunable parameter file */
#define	STUN_D	7		/* Files in stune.d directory */
#define SASN	8		/* System device variable assign file */
#define SASN_D	9		/* Files in sassign.d directory */
#define NODE	10		/* Node file */
#define NODE_D	11		/* Files in node.d directory */

/* Definition of ID file types used by getinst() */
typedef struct inst_ftype {
	char	*lname;		/* Logical name, used in error messages */
	char	*fname;		/* File or directory name */
	int	is_dir;		/* Flag: directory form instead of flat-file */
	int	basetype;	/* Base (flat-file) type */
	int	cur_ver;	/* Current syntax version number */
	int	ver;		/* Syntax version number of a file */
	FILE	*fp;		/* Open file pointer */
	DIR	*dp;		/* Open directory pointer */
} ftype_t;
extern ftype_t ftypes[];

#define NAMESZ	15	/* size includes the null character */
#define TUNESZ	21	/* size includes the null character */
#define TYPESZ	41	/* size includes the null character */
#define PFXSZ	9
#define FLAGSZ	20
#define RANGESZ	20

struct modlist {
	struct	modlist *next;
	char	name[NAMESZ];
};

struct entry_list {
	struct entry_def *edef;	/* entry_def struct for this entry point */
	struct entry_list *next;/* Next entry point for this module */
};

struct depend_list {
	struct	depend_list *next;
	char	name[NAMESZ];
};

#define MDEV_VER	1
struct mdev {			/* Master file structure for devices */
	char	name[NAMESZ];	/* device name */
	char	prefix[PFXSZ];	/* prefix for driver routines/structs */
	char	mflags[FLAGSZ];	/* letters indicating device flags */
	short	order;		/* order for init/start/execsw entries */
	short	blk_start;	/* start of multiple majors range - blk dev */
	short	blk_end;	/* end of multiple majors range - blk dev */
	short	chr_start;	/* start of multiple majors range - chr dev */
	short	chr_end;	/* end of multiple majors range - chr dev */
	struct entry_list *entries;
	char    modtype[TYPESZ];        /* module type for error message */
	struct	depend_list *depends;	/* list of depend on modules */
};

#define SDEV_VER	1
struct sdev {			/* System file structure for devices */
	char	name[NAMESZ];	/* device name */
	char	conf;		/* Y/N - Configured in Kernel */
	short	units;		/* number of units */
	short	ipl;		/* ipl level for intr handler */
	short	itype;		/* type of interrupt scheme */
	short	vector;		/* interrupt vector number */
	long	sioa;		/* start I/O address */
	long	eioa;		/* end I/O address */
	long	scma;		/* start controller memory address */
	long	ecma;		/* end controller memory address */
	short	dmachan;	/* DMA channel */
};

#define MTUNE_VER	0
struct mtune {			/* Master structure for tunable parameters */
	char	name[TUNESZ];	/* name of tunable parameter */
	long	def;		/* default value */
	long	min;		/* minimum value */
	long	max;		/* maximum value */
};

#define STUNE_VER	0
struct stune {			/* System structure for tunable parameters */
	char	name[TUNESZ];	/* name of tunable parameter */
	long	value;		/* number specified */
};

#define SASSIGN_VER	0
struct sassign {		/* System structure for dev variables */
	char	device[NAMESZ];	/* device variable name */
	char	major[NAMESZ];	/* major device name */
	long	minor;		/* minor device number */
	long	low;		/* lowest disk block in area */
	long	blocks;		/* number of disk blocks in area */
	char	objname[MAXOBJNAME];	/* pathname of object */
};

#define NODE_VER	0
struct node {			/* Structure for Node files */
	char	major[NAMESZ];	/* major device name */
	char	nodename[MAXOBJNAME];	/* pathname of node file in /dev */
	char	type;		/* BLOCK or CHAR */
	long	maj_off;	/* major number offset */
	long	minor;		/* minor number */
	char	majminor[NAMESZ];	/* minor = major of another device */
	long	uid, gid;	/* owner/group */
	unsigned short  mode;	/* file mode */
	unsigned long	level;	/* MAC-Level, should be level_t */
				/* but it's not defined in cross env */
};

/* variables for multiple major numbers */

struct	multmaj	{
	char	brange[RANGESZ];
        char	crange[RANGESZ];
};

/* Specific generic Master file flags */
#define ONCE	'o'		/* allow only one spec. of device	*/
#define REQ	'r'		/* required device			*/
#define	BLOCK	'b'		/* block type device			*/
#define	CHAR	'c'		/* character type device		*/
#define	TTYS	't'		/* device is a tty			*/
#define EXECSW	'e'		/* software exec module			*/
#define	MOD	'm'		/* STREAMS module type			*/
#define STREAM  'S'		/* STREAMS installable			*/
#define	DISP	'd'		/* dispatcher class			*/
#define UNIQ	'u'		/* same blk and char majors are assigned*/
#define FILESYS	'F'		/* filesystem module			*/
#define HARDMOD	'h'		/* non-driver hardware module		*/
#define KEEPMAJ	'k'		/* keep major numbers as is in Master	*/
#define COMPAT	'C'		/* added by idinstall for v0 compatibility */
#define FCOMPAT	'f'		/* v0 only: assume 4.0 devflag		*/


/* String buffers */
#define LINESZ	512
extern char linebuf[LINESZ];	/* current input line */
extern char errbuf[LINESZ];	/* buffer for error message */

/* Flags */
extern int ignore_entries;	/* ignore $entry lines in Master file */


int getinst(), rdinst();

/* Error codes from getinst()/rdinst()/getmajors() */

#define IERR_OPEN	-1
#define IERR_READ	-2
#define IERR_VER	-3
#define IERR_NFLDS	-4
#define IERR_FLAGS	-5
#define IERR_MAJOR	-6
#define IERR_MMRANGE	-7
#define IERR_BCTYPE	-8
#define IERR_ENTRY	-9
#define IERR_DEPEND	-10
#define IERR_LOAD	-11
#define IERR_TYPE	-12
#define IERR_STAT	-13
#define IERR_NREG	-14

/* Additional status returns from rdinst() */

#define I_MORE		-99	/* Additional input lines must be processed */

/* Error messages corresponding to above error codes */

#define EMSG_OPEN	"Error opening %s file.\n"
#define EMSG_READ	"Error reading %s file.\n"
#define EMSG_VER	"Incorrect version number in %s file.\n"
#define EMSG_NFLDS	"Wrong number of fields in %s file.\n"
#define EMSG_FLAGS	\
  "Illegal character in flags field for %s device in %s file.\n"
#define EMSG_MAJOR	\
  "Syntax error in major number for %s device in %s file.\n"
#define EMSG_MMRANGE	\
  "Invalid major range (start not less than end) for %s device in %s file.\n"
#define EMSG_BCTYPE	\
  "Type character in %s file must be 'b' for block or 'c' for character.\n"
#define EMSG_ENTRY	"Unknown entry-point name in Master file.\n"
#define EMSG_DEPEND	"Unknown dependee module name in Master file.\n"
#define EMSG_LOAD	"Unknown loadable module name in System file.\n"
#define EMSG_TYPE	"Module type name too long.\n"
#define EMSG_STAT	"Cannot get file status.\n"
#define EMSG_NREG	"Not a regular file.\n"
