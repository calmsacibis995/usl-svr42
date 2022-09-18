/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)idtools:i386/ktool/idtools/defines.h	1.6"
#ident	"$Header:"

/* Check if two line segments, (a,b) and (x,y),  overlap. */
#define	OVERLAP(a,b,x,y)	(!(b < x || a > y))
#define MAX(a, b)		(a > b ? a : b)
#define	MIN(a, b)		(a < b ? a : b)

#define BHIGH	255		/* max block device major number */
#define CHIGH	255		/* max character device major number */
#define BLOW	0		/* min block device major number */
#define CLOW	0		/* min character device major number */

/* default switch table sizes */
#define DEF_BDEV_RESERVE	20
#define DEF_CDEV_RESERVE	50
#define DEF_VFS_RESERVE		10
#define DEF_FMOD_RESERVE	50

/* loadable module types flag */
#define MODBDEV		0x1
#define MODCDEV		0x2
#define MODSTR		0x4
#define MODMOD		0x8
#define MODFS		0x10
#define MODMISC		0x20
#define MODHARD		0x40
#define MODSD		(MODSTR | MODCDEV)
#define MODSM		(MODSTR | MODMOD)
#define MODDRV		(MODBDEV | MODCDEV)
#define MODINTR		(MODDRV | MODHARD)

/* location of file */
#define	IN	0		/* file in input directory		*/
#define OUT	1		/* file in output directory		*/
#define FULL	2		/* file has full pathname		*/

/* entry-point type definition structure */
struct entry_def {
	struct entry_def *next;		/* next entry-point definition */
	int		has_sym;	/* true if module has this symbol */
	int		is_var;		/* true if this symbol is a variable */
	char		*sname;		/* space for symbol name temp str */
	char		suffix[4];	/* suffix for this type (var length) */
};

extern struct entry_def *define_entry();
extern int lookup_entry();
extern int drv_has_entry();

/* Function table definition structure */
struct ftab_def {
	struct ftab_def	*next;
	struct entry_def *entry;	/* Entry point to collect in table */
	char		*ret_type;	/* Function return type (e.g. "int") */
	char		tabname[4];	/* Table name; variable length */
};

extern struct ftab_def *define_ftab();

/* Flags for "vars" field of driver_t struct */
#define V_DEVFLAG	0x0001		/* module contains xxxdevflag var */
#define V_EXTMODNAME	0x0002		/* module contains xxxextmodname var */
#define V_MAGIC		0x0004		/* module contains xxxmagic var */
#define V_INFO		0x0008		/* module contains xxxinfo var */
#define V__TTY		0x0010		/* module contains xxx_tty var */

/* error messages */
#define OIOA	"Start I/O Address, %lx, greater than End I/O Address, %lx\n"
#define OCMA	"Start Memory Address, %lx, greater than End Memory Address, %lx\n"
#define RIOA	"I/O Address range, %lx and %lx, must be within (%lx, %lx)\n"
#define RCMA	"Controller Memory Address, %lx, must be greater than %lx\n"
#define RIVN	"Interrupt vector number, %hd, must be within (%d, %d)\n"
#define RIPL	"Interrupt priority level, %hd, must be within (%d, %d)\n"
#define RITYP	"Interrupt type, %hd, must be within (%d, %d)\n"
#define RDMA	"DMA channel, %hd, must not be greater than %hd\n"
#define CIOA	"I/O Address ranges overlap for devices '%s' and '%s'\n"
#define CCMA	"Memory Address ranges overlap for devices '%s' and '%s'\n"
#define CDMA	"DMA channel conflict between devices '%s' and '%s'\n"
#define CID	"Id '%c' shared by devices '%s' and '%s'\n"
#define IBDM	\
"Block device major number range, %hd-%hd, must be within (%d, %d)\n"
#define DBDM	\
"Identical block device major number range, %hd-%hd, for '%s' and '%s'\n"
#define ICDM	\
"Character device major number range, %hd-%hd, must be within (%d, %d)\n"
#define DCDM	\
"Identical character device major number range, %hd-%hd, for '%s' and '%s'\n"
#define UNIT	"Unit, %hd, must be within (%hd, %hd)\n"
#define MISS	"Missing required device '%s'\n"
#define ONESPEC	"Only one specification of device '%s' allowed\n"
#define TUNE	"Unknown tunable parameter '%s'\n"
#define RESPEC	"Tunable parameter '%s' respecified\n"
#define PARM	"The value of parameter '%s', %ld, must be within (%ld, %ld)\n"
#define UNK	"Unknown device '%s'\n"
#define DEVREQ	"'%s' must be a block or character device\n"
#define MINOR	"Minor device number must be within (%d, %d)\n"
#define VECDIFF	\
   "Conflicting use of interrupt vector; already used as type %d, ipl %d\n"
#define CVEC	"Interrupt vector conflict between devices '%s' and '%s'\n"
#define	OPRT	"Block device '%s' must have an 'open' function\n"
#define CLRT	"Block device '%s' must have a 'close' function\n"
#define STRAT	"Block device '%s' must have a 'strategy' function\n"
#define PRTRT	"Block device '%s' must have a 'print' function\n"
#define STRTAB	"Streams module/driver '%s' must have an 'info' structure\n"
#define TTYVAR	"Terminal driver '%s' must have an '_tty' array\n"
#define	EXRT	"Execsw module '%s' must have an 'exec' function\n"
#define	EXMAG	"Execsw module '%s' must have a 'magic' array\n"
#define	DINITRT	"Dispatcher module '%s' must have an '_init' function\n"
#define	FINITRT	"Filesystem module '%s' must have an 'init' function\n"
#define	EXIST	"Directory '%s' does not exist\n"
#define MPAR	"Missing value for tunable parameter '%s'\n"
#define FOPEN	"Can not open '%s' for mode '%s'\n"
#define WRONG	"Wrong number of fields in %s line\n"
#define SUCH	"No such device '%s' in mdevice\n"
#define TABMEM	"Not enough memory for %s table\n"
#define	CONFD	"Configured field for device '%s' must contain 'Y' or 'N'\n"
#define NOMAXMINOR "Can not find value for MAXMINOR tunable, using %lu\n"
#define RDSYM	"Error reading symbol table from %s\n"
#define TOONEW	"%s: version is too new\n"
#define ENTRYNP	"%s: specified entry-point '%s' not present\n"
#define NOCONF	"missing intermediate file for %s module\n"
#define MAJOF	"%s: %s major number used is over the switch table size\n"
