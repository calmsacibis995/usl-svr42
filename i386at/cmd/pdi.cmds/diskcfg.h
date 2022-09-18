/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:diskcfg.h	1.5"
#ident	"$Header: miked 4/6/92$"

#define	IDINSTALL	"/etc/conf/bin/idinstall"
#define	MV			"/sbin/mv"
#define	SYSTEM		"System"
#define	MASTER		"Master"
#define	SPACE		"Space.c"
#define	SPACEGEN	"space.gen"
#define	CFGNAME		"disk.cfg"
#define	SDINAME		"sdi"
#define	DCDNAME		"dcd"
#define	DCDFULLNAME	"DCD ISC HPDD"
#define	PACKD		"/etc/conf/pack.d"
#define	MAXNAMELEN	14
#define	MAXFULLNAMELEN	64
#define	MAXTYPELEN	4
#define	SCSITYPE	"SCSI"
#define	DCDTYPE		"DCD"
#define	DISKTYPE	"DISK"
#define	TAPETYPE	"TAPE"
#define	VALIDCONF	"YyNn"
#define	NOISHARE	1
#define	ISHARESAME	2
#define	ISHAREDIFF	3
#define	MINISHARE	0
#define	MAXISHARE	3
#define	ARGSPERLINE	15
#define	MAXPREFIXLEN	8

#define	SDIREPL		"_HBA_TBL" 
#define SDIREPLLEN	(sizeof(SDIREPL) - 1)
#define SDI_IPL		5

#define	DCDREPL		"_DCD_TBL"
#define DCDREPLLEN	(sizeof(DCDREPL) - 1)

/*
 * default values for the stuff in disk.cfg
 */

#define	D_DEVTYPE		""
#define	D_CAPS			""
#define	D_MEMADDR2		0x0
#define	D_IOADDR2		0x0
#define	D_MAXSEC		255
#define	D_DRIVES		2
#define	D_DELAY			10
#define	D_BASEMINOR		0
#define	D_DEFSECSIZ		512
#define	D_HBAID			7
#define	D_MAXXFER		0
#define	D_NAMEL			""
#define D_DEVICE		""
#define D_DMA2			0
#define D_IPL			0
#define D_IVEC			0
#define D_SHAR			0

/*
 * symbolic names for the supported fields
 */
#define MAX_FIELD_NAME_LEN	9

#define	CAPS		1
#define	MEMADDR2	2
#define	IOADDR2		3
#define	MAXSEC		4
#define	DRIVES		5
#define	DELAY		6
#define	BASEMINOR	7
#define	DEFSECSIZ	8
#define	HBAID		9
#define	MAXXFER		10
#define	NAMEL		11
#define DEVICE		12
#define DMA2		13
#define IPL			14
#define IVEC		15
#define SHAR		16
#define DEVTYPE		17
#define DCD_IPL			18
#define DCD_IVEC		19
#define DCD_SHAR		20

struct extra_fields {
	char			*name;	/* name of token for this field */
	unsigned short	tag;	/* number of field for case stmnt */
};

struct extra_fields field_tbl[] = {
	{"CAPS", CAPS},
	{"MEMADDR2", MEMADDR2},
	{"IOADDR2", IOADDR2},
	{"MAXSEC", MAXSEC},
	{"DRIVES", DRIVES},
	{"DELAY", DELAY},
	{"BASEMINOR", BASEMINOR},
	{"DEFSECSIZ", DEFSECSIZ},
	{"HBAID", HBAID},
	{"MAXXFER", MAXXFER},
	{"NAMEL", NAMEL},
	{"DEVICE", DEVICE},
	{"DMA2", DMA2},
	{"IPL", IPL},
	{"IVEC", IVEC},
	{"SHAR", SHAR},
	{"DEVTYPE", DEVTYPE},
	{"DCD_IPL", DCD_IPL},
	{"DCD_IVEC", DCD_IVEC},
	{"DCD_SHAR", DCD_SHAR},
};

#define NUM_FIELDS	(sizeof(field_tbl)/sizeof(struct extra_fields))

#ifndef TRUE
#define TRUE	(1)
#define MAYBE	(-1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#ifndef MAYBE
#define MAYBE	(-(TRUE))
#endif

#define DEVICE_IS_SCSI(dev)	(strcmp(dev->type,SCSITYPE)?FALSE:TRUE)
#define DEVICE_IS_DCD(dev)	(strcmp(dev->type,DCDTYPE)?FALSE:TRUE)
#define DEVICE_IS_DISK(dev)	(strcmp(dev->devtype,DISKTYPE)?FALSE:TRUE)
#define DEVICE_IS_TAPE(dev)	(strcmp(dev->devtype,TAPETYPE)?FALSE:TRUE)
#define DEVICE_IS_ACTIVE(dev)	((dev->active==0)?FALSE:TRUE)
#define DEVICE_IS_NOT_ACTIVE(dev)	((dev->active!=0)?FALSE:TRUE)
#define CONFIGURE(dev)	((dev->configure[0] == 'Y')?TRUE:FALSE)
#define EQUAL(s1,s2)	(strcmp(s1,s2)?FALSE:TRUE)
#define EQUIP_DISK	(1 << (ID_RANDOM))
#define EQUIP_TAPE	(1 << (ID_TAPE))
#define DEVICE_HAS_DISK(dev)	((dev->equip & EQUIP_DISK)?TRUE:FALSE)

struct diskdesc {
	struct diskdesc	*next;			/* Next in chain */
	/* this is read from the mdevice.d entry for this device */
	char		prefix[MAXPREFIXLEN+1];	/* Driver prefix */
	/* These are read from user input */
	int version;
	int loadable;
	char		name[MAXNAMELEN+1];	/* Name of driver */
	char		fullname[MAXFULLNAMELEN+1];	/* Human-readable name */
	char		type[MAXTYPELEN+1];	/* Adapter type */
	char		configure[2];		/* Configure device? */
	int	unit;						/* unit field from sdevice */
	unsigned short	dma1;			/* DMA channel 1 */
	unsigned short	dma2;			/* DMA channel 2 */
	unsigned short	ipl;			/* Interrupt priority */
	unsigned short	ivect;			/* Interrupt vector */
	unsigned short	ishare;			/* Interrupt sharing */
	unsigned long	sioaddr;		/* Start I/O addr */
	unsigned long	eioaddr;		/* End I/O addr */
	unsigned long	smemaddr;		/* Start memory addr */
	unsigned long	ememaddr;		/* End memory addr */
	/* These are read from the disk.cfg file for this device */
	char			*devtype;		/* DCD device type ( DISK or TAPE ) */
	char			*caps;			/* Driver Capability Flags as a string */
	unsigned long	memaddr2;		/* secondary memory addr */
	unsigned long	ioaddr2;		/* secondary I/O addr */
	unsigned short	maxsec;			/* Max # of sectors in single controller req */
	unsigned short	drives;			/* Max number of drives on controller */
	unsigned short	delay;			/* Delay time for switching drives in 10us units */
	unsigned short	baseminor;		/* First entry in 'minormap' to be used */
	unsigned short	defsecsiz;		/* Default sector size for drives on this cntrlr */
	unsigned short	hbaid;			/* Host Bus Adapter SCSI ID - usually 7 */
	unsigned long	maxxfer;		/* HBA max dma size at one time */
	/* This is derived */
	unsigned long	equip;			/* devices on this HBA */
	unsigned long	order;			/* position of this device in the HBA */
	unsigned long	active;			/* is this device active? */
	int	occurence;			/* Unit number */
	char		*Install_path;			/* path to idinstall source directory */
};
