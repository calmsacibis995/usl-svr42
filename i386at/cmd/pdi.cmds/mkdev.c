/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:mkdev.c	1.6.3.16"
#ident  "$Header: miked 6/3/92$"

/*
 * pdimkdev [-d tcindexfile] [-s] [-f] [-i] [-S]
 *
 * /etc/scsi/pdimkdev is a utility that creates device nodes for
 * equipped PDI devices. It determines the device equippage
 * by examining the edt. Since the device 
 * nodes that are created for each device are unique to that 
 * device type, template files are used to specify the device 
 * naming conventions. The location of the template files is
 * specified in a target controller index file which may be 
 * supplied as a command line argument. The display of a new
 * device can also be controlled by an argument.
 *
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/statfs.h>
#include	<ctype.h>
#include	<sys/errno.h>
#include	<sys/stat.h>
#include	<sys/signal.h>
#include	<sys/mkdev.h>
#include	<sys/fcntl.h>
#include	<sys/buf.h>
#include	<sys/vtoc.h>
#include	<string.h>
#include 	<ftw.h>

#include	<devmgmt.h>
#include	<unistd.h>
#include	<libgen.h>
#include	<sys/vfstab.h>
#include	<sys/sd01_ioctl.h>
#include	<sys/fs/s5param.h>
#include	<sys/fs/s5filsys.h>

#include	<sys/sdi_edt.h>
#include	<sys/scsi.h>
#include	<sys/sdi.h>
#include	<sys/scsicomm.h>

#define NO_CHANGES_EXIT 1
#define	CMDNAME		"pdimkdev"
#define	MKDTABNAME	"pdimkdtab"
#define	TCINDEX		"/etc/scsi/tc.index"
#define DEVFILE		"/tmp/pdidevXXXXXX"
#define LAST_BOOT_EDT	"/etc/scsi/pdi_edt"
#define TEMP_EDT	"/etc/scsi/tmp.edt"
#define INSTALLF_INPUT	"/etc/scsi/installf.input"
#define MASK		00644
#define BLANK		'-'
#define YES		1
#define NO		0
#define DEV_MODE	0600
#define DEV_UID		0	/* owner root */
#define DEV_GID		3	/* group sys */
#define DIRMODE		0775
#define	MAXLINE		256
#define MAXMAJOR	255
#define	MAXTCTYPE	32
#define INODECRITICAL	50
#define INODELOW	200
#define	BIG		017777777777L
#define	BIG_I		2000

/* definitions of template file fields */
#define KEY		0
#define MINOR		1
#define MODE		2
#define	BLOCK		3
#define CHAR		4
#define SA		5
#define RSA		6
#define NUMFIELDS	7 /* the total number of fields in the template file */

/* Type Definitions */
typedef	struct	stat		STAT;
typedef	struct	scsi_edt	EDT;

typedef struct  tokens {
	char string[32];
	int  token;
} TOKENS_T;

typedef struct  devices {
	char string[32];
} DEVICE_T;

typedef struct  drivers {
	char name[NAME_LEN];
	int	subdev;
} DRIVER_T;

struct scsi_addr{
	unsigned int	sa_c;		/* controller occurrence        */
	unsigned int	sa_t;		/* target controller number     */
	unsigned int	sa_l;		/* logical unit number          */
	unsigned int	sa_n;		/* admin name unit number       */
};

extern char	*calloc(),
		*sys_errlist[];
extern int	errno,
		mkdir(),
		statfs();
extern void	exit(),
		error(),
		warning();

/* Functions local to this file */
extern void	CreateDirectory(),
		E333A_nodes(),
		MakeDeviceNodes(),
		MakeSCSIMajors();
extern uint	CalculateMinorNum();
extern FILE	*OpenTemplate();
extern int	BootedOffSCSI(),
		FileExists(),
		GetToken(),
		MakeAdminName(),
		MakeNodeName(),
		NumFreeInodes(),
		RemoveSCSINodes(),
		RemoveMatchingNode();

/* Static Variables */
static char	Pdimkdev = FALSE;
static char	Silent = FALSE;
static char	TempDev[] = DEVFILE; /* temporary filename template */
static char	Cmdname[64];
static char	TCindex[128];
static int	Freeinodes;
static FILE	*contents_fp;
static FILE	*TCindexfp;
static FILE	*last_edt_fp;
static FILE	*temp_edt_fp;
static char	TC_err[] =	"format of the target controller index file";
static int	Booted_off_scsi = FALSE;
static char	SCSICMajors[MAXMAJOR + 1];
static char	SCSIBMajors[MAXMAJOR + 1];
static char	SCSI_name[49];
int	Debug;

/* Token Definitions:
 *	To add a token, add a define below and update NUMTOKENS. Then
 *	add the string and token to the Tokens array that follows.
 */
#define MKDEV		0
#define TCTYPE		1
#define QUERY		2
#define POSTMSG		3
#define COMMENT		4
#define DATA		5
#define TCINQ		6
#define ALIAS		7
#define DGRP 		8
#define ATTR 		9
#define FSATTR 		10
#define DPATTR 		11
#define GENERIC		12
#define UNKNOWN		13
#define NUMTOKENS	14 /* Should be one beyond maximum number of tokens */
#define UNTOKEN		-2

TOKENS_T	Tokens[] = {
	"MKDEV",	MKDEV,
	"TCTYPE",	TCTYPE,
	"QUERY",	QUERY,
	"POSTMSG",	POSTMSG,
	"#",		COMMENT,
	"DATA",		DATA,
	"TCINQ",	TCINQ,
	"ALIAS",	ALIAS,
	"DGRP",		DGRP,
	"ATTR",		ATTR,
	"FSATTR",	FSATTR,
	"DPATTR",	DPATTR,
	"GENERIC",	GENERIC,	
};

DEVICE_T	Device_Types[] = {
	"Disk",			/*	ID_RANDOM	*/
	"Tape",			/*	ID_TAPE		*/
	"Printer",		/*	ID_PRINTER	*/
	"Processor",		/*	ID_PROCESSOR	*/
	"WORM",			/*	ID_WORM		*/
	"ROM",			/*	ID_ROM		*/
	"Scanner",		/*	ID_SCANNER	*/
	"Optical",		/*	ID_OPTICAL	*/
	"Changer",		/*	ID_CHANGER	*/
	"Communication",	/*	ID_COMMUNICATION*/
	"Host Adapter",		/*	HOST_ADAPTER	*/
};
#define HOST_ADAPTER 10

#define NUMDRVRS 12	/* number of entries in SCSIDrivers array */
#define SCSI	0	/* must match array below */
#define MIRROR	1	/* must match array below */
#define SD01	2	/* must match array below */
#define ST01	4	/* must match array below */
#define SDI 	9	/* must match array below */
#define SC01	10	/* must match array below */
#define SW01	11	/* must match array below */

/* The entries below show the driver name */
DRIVER_T	SCSIDrivers[] = {
	"scsi",SCSI_SUBDEVS,		/* Host Adapter */
	"mirror",SCSI_SUBDEVS,	/* mirroring	*/
	"sd01",16,				/* hard disk	*/
	"st00",SCSI_SUBDEVS,		/* 9-Track tape	*/
	"st01",16,				/* cart. tape */
	"st02",SCSI_SUBDEVS,		/* cart. tape	*/
	"so00",SCSI_SUBDEVS,		/* optical disk	*/
	"sc00",SCSI_SUBDEVS,		/* changer	*/
	"sr00",SCSI_SUBDEVS,		/* cd-rom	*/
	"sdi",SCSI_SUBDEVS,		/* Host Adapter */
	"sc01",1,				/* cd-rom	*/
	"sw01",1,				/* worm		*/
};

struct HBA {
	int	index;		/* controller number of the c'th HBA */
	int	order;		/* translation from HBA order to device order */
	EDT	*edtptr;	 			/* ptr to HBA[c]'s EDT starting point */
} HBA[MAX_HAS];

struct DRVRinfo {
	int	valid;
	char	name[NAME_LEN];	/* driver name			*/
	struct drv_majors *majors;	/* array of all major sets	*/
	int	majsets;	/* count of all major sets	*/
	int	subdevs;	/* number of subdevices per LU	*/
	int	lu_limit;
	int	lu_occur;	/* occurrence of the current LU */
	int	admin_count; /* how many of each of these exist per HA */
	struct drv_majors *lu_majors;	/* major set for the current LU */
} DRVRinfo[MAXTCTYPE];



void
before_exit()
{
	char system_line[MAXLINE];
	long	length;

	if (strcmp(Cmdname, MKDTABNAME) == 0)
		return;

	/* last ditch attemt to make basic nodes when booted off SCSI */
	if (Booted_off_scsi == TRUE)
		E333A_nodes(SCSI_SUBDEVS);

	length = ftell(contents_fp);
	fclose(contents_fp);
	if ( length ) {
		sprintf(system_line,"installf base - >/dev/null 2>&1 < %s",INSTALLF_INPUT);
		(void)system(system_line);
	}
	(void)unlink(INSTALLF_INPUT);
}


/*
 * This routine writes a scsi address string to the location provided.
 */
void
scsi_name(sa, message)
struct scsi_addr	*sa;	/* scsi address pointer  */
char	*message;	/* Message to be reported */
{
	if (sa == NULL) {
		sprintf(message, "");
	} else {
		/* occurrence based addressing */
		sprintf(message, "HA %u TC %u LU %u",sa->sa_c, sa->sa_t, sa->sa_l);
	}
}

/*
 * Check for the existence and correctness of a special device. Returns TRUE if
 *	the file exists and has the correct major and minor numbers
 *	otherwise it returns FALSE, warns for error.
 */
int
SpecialExists(type,path,dev_major,dev_minor)
int type,dev_major,dev_minor;
char *path;
{
	STAT	statbuf;

	if (lstat(path,&statbuf) < 0) {
		/* 
		 * errno == ENOENT if file doesn't exist.
		 * Otherwise exit because something else is wrong.
		 */
		if (errno != ENOENT) {
			warning("%s: stat failed for %s\n",Cmdname, path);
			return(TRUE);
		} else {/* file does not exist */
			return(FALSE);
		}
	}

	/* file exists */

	statbuf.st_mode &= S_IFMT;

	/* do it's major and minor numbers, match the one we need */

	if ( ((statbuf.st_mode & S_IFCHR) == S_IFCHR) && type == CHAR ) {
		if (major(statbuf.st_rdev) == dev_major && minor(statbuf.st_rdev) == dev_minor) {
			return(TRUE);
		}
	} else if ( ((statbuf.st_mode & S_IFBLK) == S_IFBLK) && type == BLOCK ) {
		if (major(statbuf.st_rdev) == dev_major && minor(statbuf.st_rdev) == dev_minor) {
			return(TRUE);
		}
	}

	return(FALSE);
}


/*
 * Check for the existence of a file. Returns TRUE if the file exists,
 * returns FALSE if the file doesn't exist, warns for error.
 */
int
FileExists(path)
char *path;
{
	STAT	statbuf;

	if (stat(path,&statbuf) < 0) {
		/* 
		 * errno == ENOENT if file doesn't exist.
		 * Otherwise exit because something else is wrong.
		 */
		if (errno != ENOENT) {
			warning("%s: stat failed for %s\n",Cmdname, path);
			return(TRUE);
		}
		else /* file does not exist */
			return(FALSE);
	}

	/* file exists */
	return(TRUE);
}


/* For performance a boolean array is used to indicate whether a
 * SCSI driver is present for each major number.
 */
void
MakeSCSIMajors()
{
	int i, j;
	struct drv_majors *majors;
	int majsets;
	int debugj, debugk;

	if(Debug) printf("MakeSCSIMajors()\n");
	/* inititalize the array */
	for(i=0;i<=MAXMAJOR;i++) {
		SCSICMajors[i]=FALSE;
		SCSIBMajors[i]=FALSE;
	}

	/*
	 * Add the major numbers for drivers found in the EDT to the major
	 * number list.
	 */
	for(i=0;i<MAXTCTYPE;i++) {
		if (DRVRinfo[i].valid == FALSE) /* No more majors */
			break;		

		majors = DRVRinfo[i].majors;
		for(j=0; j<DRVRinfo[i].majsets; j++) {
			if(majors->c_maj != NO_MAJOR)
				SCSICMajors[majors->c_maj] = TRUE;
			if(majors->b_maj != NO_MAJOR)
				SCSIBMajors[majors->b_maj] = TRUE;
			majors++;
		}
	}	

	/* 
	 * Add the major numbers of known SCSI drivers to the major number list.
	 * If any of these drivers are in the mdevice file, but no devices
	 * show in the EDT, then any old nodes for that driver will get zapped.
	 */
	for(i=0;i<NUMDRVRS;i++) {
		if ((majors = GetDriverMajors(SCSIDrivers[i].name, &majsets)) != 0) {
			for(j=0; j<majsets; j++) {
				if(majors->c_maj != NO_MAJOR)
					SCSICMajors[majors->c_maj] = TRUE;
				if(majors->b_maj != NO_MAJOR)
					SCSIBMajors[majors->b_maj] = TRUE;
				majors++;
			}
		}
	}

	if(Debug) {
		printf("\n");
		for(debugj=0; debugj<4; debugj++) {
			printf("cmaj ");
			for (debugk=0;debugk < 64;debugk++) {
				printf("%d",(int) SCSICMajors[debugj*64 + debugk]);
				if (((debugk+1) % 8) == 0)
					printf(" ");
			}
			printf("\n");
		}
		printf("\n");
		for(debugj=0; debugj<4; debugj++) {
			printf("bmaj ");
			for (debugk=0;debugk < 64;debugk++) {
				printf("%d",(int) SCSIBMajors[debugj*64 + debugk]);
				if (((debugk+1) % 8) == 0)
					printf(" ");
			}
			printf("\n");
		}
	}
}

int
RemoveSCSINodes(path)
char *path;
{
	int rc;
	rc = ftw(path, RemoveMatchingNode, 4);
	return(rc);
}

/*
 * the RemoveMatchingNode routine is called from the RemoveSCSINodes routine.
 * It is used in a call to ftw() which will recursively descend a directory and
 * execute RemoveMatchingNode for each file in that directory tree.
 * It compares the major number of each file to a list of SCSI major numbers.
 * Files matching those major numbers are removed. 
 */
int 
RemoveMatchingNode(name, statbuf, code)
char *name;
STAT *statbuf;
int code;
{
	short found, maj;
	found = FALSE;
	switch(code) {

	case FTW_F:
		/* check file against list of major numbers */

		maj = major(statbuf->st_rdev);
		switch(statbuf->st_mode & S_IFMT) {

			case S_IFCHR:
				found = SCSICMajors[maj];
				break;
			case S_IFBLK:
				found = SCSIBMajors[maj];
				break;
			default:	/* not a special file */
				return(0);
				/* NOTREACHED */
				break;
		}

		if (found) {
			if(Debug && (maj == 0))
				printf("removing node with major zero : %s\n",name);
			unlink(name);
			Freeinodes++;
			return(0);
		}
		break;
	default:
		break;
	}
	return(0);
}


int
NumFreeInodes()
{
	struct statfs	statfsbuf;

	if(Debug) printf("NumFreeInodes()\n");
	if (statfs("/", &statfsbuf, (sizeof (struct statfs)),0) < 0 ) {
		warning("%s: Could not statfs the root file system.\n", Cmdname);
		/* make up a number so that pdimkdev can continue.  */
		return(BIG_I);
	}

	if (Debug) printf("File system: %s,inodes free: %d\n",statfsbuf.f_fname, statfsbuf.f_ffree);
	return(statfsbuf.f_ffree);
	/*NOTREACHED*/
}


/* GetToken() - reads the SCSI template file and returns the token found */
int
GetToken(templatefp)
FILE *templatefp;	/* File pointer for the template file */
{
	char	token[MAXLINE];
	int	curtoken;

	/* Read the next token from the template file */
	switch (fscanf(templatefp,"%s",token)) {
	case EOF :
		return(EOF);
	case 1	: /* normal return */
		break;
	default :
		return(UNKNOWN);
		/*NOTREACHED*/
		break;
	}

	/* Determine which token */
	for (curtoken = 0; curtoken < NUMTOKENS; curtoken++) {
		if (strcmp(Tokens[curtoken].string,token) == 0) {
			return(Tokens[curtoken].token);
		}
	}

	/* allow comment tokens to not be separated by white space from text */
	if (strncmp(Tokens[COMMENT].string,token,strlen(Tokens[COMMENT].string)) == 0)
		return(Tokens[COMMENT].token);

	/* Token not found */
	return(UNTOKEN);
}	/* GetToken() */


FILE *
OpenTemplate(tc_pdtype,tcinq)
unsigned char	tc_pdtype;
char	*tcinq;
{
	register int	tctype_match, templatefound, tokenindex, i;
	FILE	*templatefp;
	int	end;
	char	templatefile[MAXFIELD], tctypestring[MAXFIELD], gfile[MAXFIELD];
	char	tcinqstring[INQ_LEN], tctypetoken[MAXFIELD];
	int	num_pdtype;
	int	gflag;
	unsigned char	generic_type;
	TOKENS_T	Generics[] = {
		"NO_DEV",	ID_NODEV,
		"RANDOM",	ID_RANDOM,
		"TAPE",		ID_TAPE,
		"PRINTER",	ID_PRINTER,
		"PROCESSOR",	ID_PROCESOR,
		"WORM",		ID_WORM,
		"ROM",		ID_ROM,
		"SCANNER",	ID_SCANNER,
		"OPTICAL",	ID_OPTICAL,
		"CHANGER",	ID_CHANGER,
		"COMMUNICATION",	ID_COMMUNICATION,
		"HOST_ADAPTER",	HOST_ADAPTER,
	};

	templatefile[0] = '\0';
	tctypestring[0] = '\0';
	tcinqstring[0]  = '\0';
	tctypetoken[0]  = '\0';
	num_pdtype = 12;
	gflag = 0;
	gfile[0] = '\0';

	strcpy(tcinqstring, tcinq);
	/* pad inquiry string with blanks if necessary */
	if (strlen(tcinqstring) != (VID_LEN + PID_LEN)) {
		end = strlen(tcinqstring);
		for(i = end; i < (VID_LEN + PID_LEN); i++)
			tcinqstring[i] = ' ';
		tcinqstring[VID_LEN + PID_LEN] = '\0';
	}
	tctype_match = FALSE;
	templatefound = FALSE;
	generic_type = UNKNOWN;

	/* start at the beginning of the tcindex file */
	rewind(TCindexfp);
	while (!templatefound) {
		tokenindex = GetToken(TCindexfp);
		switch (tokenindex) {
		/* A new TC INQuiry token was added to allow the TC's inquiry 
		 * string to specify the devices template file.
		 */
		case TCINQ:
			if (fscanf(TCindexfp," %[^\n]\n",tctypetoken) == EOF) {
				errno = 0;
				warning("%s: %s : %s\n", Cmdname,TC_err,TCindex);
				return(NULL);
			}
			/* pad inquiry string with blanks if necessary */
			if (strlen(tctypetoken) != (VID_LEN + PID_LEN)) {
				end = strlen(tctypetoken);
				for(i = end; i < (VID_LEN + PID_LEN); i++)
					tctypetoken[i] = ' ';
				tctypetoken[VID_LEN + PID_LEN] = '\0';
			}
			/* Compares given length of vendor and product names */
			if (strcmp(&tctypetoken[0 + VID_LEN],&tcinqstring[0 + VID_LEN]) == 0)
				tctype_match = TRUE;
			break;

		case MKDEV:
			if (tctype_match) {
				if(fscanf(TCindexfp," %[^\n]\n",templatefile) == EOF) {
					errno = 0;
					warning("%s: %s : %s\n", Cmdname,TC_err,TCindex);
					return(NULL);
				}
				templatefound = TRUE;
			}
			else {
				if (gflag) {
					if(fscanf(TCindexfp," %[^\n]\n",gfile) == EOF) {
						errno = 0;
						warning("%s: %s : %s\n", Cmdname,TC_err,TCindex);
						return(NULL);
					}
					gflag--;
				}
				else 
					/* read the remainder of the input line */
					fscanf(TCindexfp,"%*[^\n]%*[\n]");
			}
			break;
		case GENERIC:
			if (!tctype_match) {
				if(fscanf(TCindexfp," %[^\n]\n",tctypestring) == EOF) {
					errno = 0;
					warning("%s: %s : %s\n", Cmdname,TC_err,TCindex);
					return(NULL);
				}
				for (i=0; i < num_pdtype; i++) 
					if (strcmp(Generics[i].string,tctypestring) == 0) {
						generic_type = Generics[i].token;
						break;
					}
				if (tc_pdtype == generic_type)
					gflag++;
			}
			break;
		case EOF:
			if (!tctype_match && gfile[0] != '\0' ) {
				strcpy (templatefile, gfile);
				templatefound = TRUE;
				break;
			}
			else {
				errno = 0;
				warning("%s: TC entry not found in %s for \"%s\".\n", Cmdname,TCindex, tcinq);
				return(NULL);
			}
		case COMMENT:
		case UNTOKEN:
		case UNKNOWN:
		default:       /* read the remainder of the input line */
			fscanf(TCindexfp,"%*[^\n]%*[\n]");
			break;
		}
	}
	/* Check to see that the template file exists. */
	if (!FileExists(templatefile)) {
		warning("%s: template file %s does not exist.\n", Cmdname,templatefile);
		return(NULL);
	}
	/* Open the target controller template file. */
	if ((templatefp = fopen(templatefile,"r")) == NULL) 
		warning("%s: Could not open %s.\n", Cmdname,templatefile);
	
	return(templatefp);
}

/*
 * Convert a numeric string arg to binary				
 * Arg:	string - pointer to command arg					
 *									
 * Always presume that operators and operands alternate.		
 * Valid forms:	123 | 123*123 | 123+123 | L*16+12			
 * Return:	converted number					
 */									
unsigned int
CalculateMinorNum(token, sa, drvr)
register char *token;
struct scsi_addr *sa;
int drvr;
{
/*
 * The BIG parameter is machine dependent.  It should be a long integer
 * constant that can be used by the number parser to check the validity	
 * of numeric parameters.  On 16-bit machines, it should probably be	
 * the maximum unsigned integer, 0177777L.  On 32-bit machines where	
 * longs are the same size as ints, the maximum signed integer is more	
 * appropriate.  This value is 017777777777L.				
 */
	register char *cs;
	long n;
	long cut = BIG / 10;	/* limit to avoid overflow */

	cs = token;
	n = 0;
	/* check for operand */
	switch (*cs) {
	case 'C':
		n = sa->sa_c;
		cs++;
		break;
	case 'T':
		n = sa->sa_t;
		cs++;
		break;
	case 'L':
		n = sa->sa_l;
		cs++;
		break;
	case 'D':
		n = DRVRinfo[drvr].lu_occur;
		cs++;
		break;
	case 'S':
		n = DRVRinfo[drvr].subdevs;
		cs++;
		break;
	default:
		while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
			n = n*10 + *cs++ - '0';
	}

	/* then check for the subsequent operator */
	switch (*cs++) {

	case '+':
		n += CalculateMinorNum(cs,sa,drvr);
		break;
	case '*':
	case 'x':
		n *= CalculateMinorNum(cs,sa,drvr);
		break;

	/* End of string, check for a valid number */
	case '\0':
		if ((n > BIG) || (n < 0)) {
			before_exit();
			errno = 0;
			scsi_name(sa, SCSI_name);
			error("%s: minor number out of range for %s\n", Cmdname, SCSI_name);
		}
		return(n);
		/*NOTREACHED*/
		break;

	default:
		before_exit();
		errno = 0;
		error("%s: bad token in template file: \"%s\"\n",Cmdname, token);
		break;
	}

	if ((n > BIG) || (n < 0)) {
		before_exit();
		errno = 0;
		scsi_name(sa, SCSI_name);
		error("%s: minor number out of range for %s\n", Cmdname, SCSI_name);
	}

	return(n);
	/*NOTREACHED*/
}

/*
 * Using the directory and the token, substitute the ha, tc, and lu
 * numbers for the corresponding key letters in the token and concatenate
 * with the directory name to make the full path name of the device.
 * Return 1 if successful, return zero and a null devname if not.
 */
int
MakeNodeName(devname,dir,token,sa)
char *devname;
char *dir;
char *token;
struct scsi_addr *sa;
{
	register int i;
	char c;

	devname[0] = '\0';
	if ((dir[0] == BLANK) || (token[0] == BLANK))
		return(0);

	i = 0;
	(void) strcat(devname,dir);
	while ( (c = token[i++]) != '\0') {

		switch (c) {
		case 'C':
			(void) sprintf(devname,"%s%d",devname,sa->sa_c);
			break;
		case 'T':
			(void) sprintf(devname,"%s%d",devname,sa->sa_t);
			break;
		case 'L':
			(void) sprintf(devname,"%s%d",devname,sa->sa_l);
			break;
		default :
			(void) sprintf(devname,"%s%c",devname,c);
			break;
		}
	}
	return(1);
}

/*
 * Using the directory and the token, substitute the ha, tc, and lu
 * numbers for the corresponding key letters in the token and concatenate
 * with the directory name to make the full path name of the device.
 * Return 1 if successful, return zero and a null devname if not.
 */
int
MakeAdminName(devname,dir,token,sa)
char *devname;
char *dir;
char *token;
struct scsi_addr *sa;
{
	register int i;
	register char c;

	devname[0] = '\0';
	if ((dir[0] == BLANK) || (token[0] == BLANK))
		return(0);

	i = 0;
	(void) strcat(devname,dir);
	while ( (c = token[i++]) != '\0') {

		switch (c) {

		case '\\':
			if ( token[i] != '\0' ) {
				c = token[i++];
				(void) sprintf(devname,"%s%c",devname,c);
			}
			break;

		case 'N':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n);
			break;
		case 'C':
			(void) sprintf(devname,"%s%d",devname,sa->sa_c);
			break;
		case 'T':
			(void) sprintf(devname,"%s%d",devname,sa->sa_t);
			break;
		case 'L':
			(void) sprintf(devname,"%s%d",devname,sa->sa_l);
			break;
		default :
			(void) sprintf(devname,"%s%c",devname,c);
			break;
		}
	}
	return(1);
}

/* 
 * The CreateDirectory routine takes a directory path argument and creates
 * that directory if it does not yet exist. Error handling is not
 * performed since any errors in stat or mkdir that are ignored here
 * would be handled in the MakeDeviceNodes routine anyway.
 */
void
CreateDirectory(dir)
char *dir;
{
	STAT	statbuf;
	char	newdir[MAXFIELD], tmpdir[MAXFIELD];
	char	*newdirp, *tok;

	/* check to see if directory field is blank */
	if (dir[0] == BLANK)
		return;

	/* check to see if the directory already exists */
	if (stat(dir,&statbuf) == 0)
		return;
	
	/*
         * Now start at beginning of path and create each
	 * directory in turn.
	 */
	strcpy(newdir,dir);
	newdirp=newdir;
	strcpy(tmpdir,"");
	while( (tok=strtok(newdirp,"/")) != NULL) {
		newdirp=NULL; 		/* set to null for next call to strtok */
		strcat(tmpdir,"/");
		strcat(tmpdir,tok);
		if (stat(tmpdir, &statbuf) < 0) {
			mkdir(tmpdir,DIRMODE);
		}
	}
}


/*
 * The MakeDeviceNodes routine is called for every logical unit. 
 * It checks the free inodes and then makes the device nodes.
 */
void
MakeDeviceNodes(sa,drvr,tcinq,tc_pdtype, bus_version)
struct scsi_addr *sa;
register int	drvr;	/* DRVRinfo index */
char	*tcinq;
unsigned char tc_pdtype;
int bus_version;
{
	short 		tc_cmajor, tc_bmajor;
	register int	p, tokenindex, lowinode, lu_occur;
	int		create, reply, data_begin, minornum, modenum;
	char		field[NUMFIELDS][MAXFIELD], dir[NUMFIELDS][MAXFIELD];
			/*
			 * the devname array below is used to hold the block,
			 * character, SA, and rSA filenames for the device.
			 * The values in these fields correspond to the current
			 * line being examined in the device template file
			 */
	char		devname[NUMFIELDS][MAXFIELD];
	char		query[MAXLINE], answer[MAXFIELD];
	char		tempmsg[MAXLINE], postmsg[MAXLINE];
	FILE		*templatefp;
	int		NodesCreated;
	
	if (Debug) {
	scsi_name(sa, SCSI_name);
	printf("MakeDeviceNodes:%s occ=%d\n", SCSI_name, DRVRinfo[drvr].lu_occur);
	}

	create = TRUE;
	reply = YES;
	lowinode = FALSE;
	lu_occur=DRVRinfo[drvr].lu_occur;
	postmsg[0]='\0';


	/* if this is an unknown tctype then simply return */
	if ((templatefp = OpenTemplate(tc_pdtype,tcinq)) == NULL)
		return;

	/* return if the occurrence is out of bounds */
	if (lu_occur >= DRVRinfo[drvr].lu_limit) {
		errno=0;
		warning("%s: Found %d %s devices in the configuration.\nThe %s driver is configured to support %d devices.\nConsult your SCSI documentation for reconfiguration procedures.\n", Cmdname,lu_occur + 1, Device_Types[tc_pdtype].string,Device_Types[tc_pdtype].string, DRVRinfo[drvr].lu_limit);
		
		return;
	}

	data_begin=FALSE;
	while (!data_begin) {
		tokenindex = GetToken(templatefp);
		switch (tokenindex) {
		case QUERY:
			fscanf(templatefp, " %[^\n]\n", query);
			break;
		case POSTMSG:
			fscanf(templatefp, " %[^\n]\n", tempmsg);
			strcat(postmsg, tempmsg);
			strcat(postmsg, "\n");
			break;
		case DATA:
			data_begin=TRUE;
			break;
		case EOF:
			(void) fclose(templatefp);
			return;
			/* NOTREACHED */
			break;
		default:
		case COMMENT:
			fscanf(templatefp, "%*[^\n]%*[\n]");
			break;
		}
	}

	/* 
	 * The next line contains the device directories in cols 4 to 7
	 */
	(void)ParseLine(dir,templatefp,NUMFIELDS);

	/*
	 * Create directories if they don't already exist.
	 */
	CreateDirectory(dir[BLOCK]);
	CreateDirectory(dir[CHAR]);
	CreateDirectory(dir[SA]);
	CreateDirectory(dir[RSA]);

	/* 
	 * Check number of inodes, exit if below critical point,
	 * query if below low water mark.
	 */
	if (Freeinodes < INODECRITICAL) {
		before_exit();
		errno = 0;
		error("%s: Only %d free files remain.\nNot enough files available to create device nodes.\nTry removing unused files from the root file system.\n",Cmdname, Freeinodes);
	} else if (Freeinodes < INODELOW ) {
		lowinode = TRUE;
		errno = 0;
		warning("%s: The root file system is becoming low on files.\nOnly %d free files remain.\n", Cmdname,Freeinodes);
		if (query[0] != BLANK) {
			printf("\nCreating nodes for the device shown below.\n");
			printf("Host Adapter (HA)          = %d\n",sa->sa_c);
			printf("Target Controller (TC) ID  = %d\n",sa->sa_t);
			printf("Logical Unit (LU) ID       = %d\n",sa->sa_l);
			printf("%s [yes]\n",query);
			(void) gets(answer);
		}
		if ((answer[0] == 'Y') || (answer[0] == 'y')) {
			reply = YES;
		} else {
			reply = NO;
		}
	} 

	NodesCreated = FALSE;

	/* read input lines for each subdevice, then make the device nodes */
	while ((p = ParseLine(field,templatefp,NUMFIELDS)) != EOF) {

		if (p != NUMFIELDS) {
			break; /* corrupted template file */
		}

		if (lowinode) {
			create = FALSE;
			if (strchr(field[KEY], 'M') != NULL) {
				create = TRUE;
			} else if (strchr(field[KEY], 'Y') != NULL) {
				if (reply == YES)
					create = TRUE;
			}
		}

		minornum = CalculateMinorNum(field[MINOR],sa,drvr);
		modenum = (int) strtol(field[MODE],(char **)NULL,8);

		/* generate the special device file names */
		MakeNodeName(devname[BLOCK],dir[BLOCK],field[BLOCK],sa);
		MakeNodeName(devname[CHAR],dir[CHAR],field[CHAR],sa);

		if ( !Pdimkdev ) {
			/* Make the OA&M device table entry */
			if (strchr(field[KEY], 'O') != NULL)
				MakeDeviceTable(devname[BLOCK], devname[CHAR], tcinq, bus_version,tc_pdtype);
			continue;
		}

		tc_cmajor = DRVRinfo[drvr].lu_majors->c_maj;
		tc_bmajor = (DRVRinfo[drvr].lu_majors)->b_maj;

		if ((create) && (devname[BLOCK][0] != '\0') && (tc_bmajor != NO_MAJOR)){
			if (!SpecialExists(BLOCK,devname[BLOCK],tc_bmajor,minornum)) {
				(void) unlink(devname[BLOCK]);
				if (mknod(devname[BLOCK],modenum | S_IFBLK,
							makedev(tc_bmajor,minornum)) < 0) {
					warning("%s: mknod failed for %s.\n",
							Cmdname,devname[BLOCK]);
				} else {
					NodesCreated = TRUE;
					Freeinodes--;
					chown(devname[BLOCK],(uid_t)DEV_UID,(gid_t)DEV_GID);
					fprintf(contents_fp,"%s b %d %d ? ? ?\n",devname[BLOCK],tc_bmajor,minornum);
		       	}
			}
		}
		if ((create) && (devname[CHAR][0] != '\0') && (tc_cmajor != NO_MAJOR)) {
			if (!SpecialExists(CHAR,devname[CHAR],tc_cmajor,minornum)) {
				(void) unlink(devname[CHAR]);
				if (mknod(devname[CHAR],modenum | S_IFCHR,
							makedev(tc_cmajor,minornum)) < 0) {
					warning("%s: mknod failed for %s.\n",
							Cmdname,devname[CHAR]);
				} else {
					NodesCreated = TRUE;
					Freeinodes--;
					chown(devname[CHAR],(uid_t)DEV_UID,(gid_t)DEV_GID);
					fprintf(contents_fp,"%s c %d %d ? ? ?\n",devname[CHAR],tc_cmajor,minornum);
		        }
			}
		}

		/* link the system admin names if necessary */
		if (tc_bmajor != NO_MAJOR &&
			MakeAdminName(devname[SA],dir[SA],field[SA],sa)) {
			if (!SpecialExists(BLOCK,devname[SA],tc_bmajor,minornum)) {
				(void) unlink(devname[SA]);
				if (link(devname[BLOCK],devname[SA]) < 0) {
					warning("%s: %s\n", Cmdname,devname[SA]);
				} else {
					NodesCreated = TRUE;
					Freeinodes--;
					chown(devname[BLOCK],(uid_t)DEV_UID,(gid_t)DEV_GID);
					fprintf(contents_fp,"%s b %d %d ? ? ?\n",devname[SA],tc_bmajor,minornum);
				}
			}
		}

		if (tc_cmajor != NO_MAJOR &&
			MakeAdminName(devname[RSA],dir[RSA],field[RSA],sa)) {
			if (!SpecialExists(CHAR,devname[RSA],tc_cmajor,minornum)) {
				(void) unlink(devname[RSA]);
				if (link(devname[CHAR],devname[RSA]) < 0) {
					warning("%s: %s\n", Cmdname,devname[RSA]);
				} else {
					NodesCreated = TRUE;
					Freeinodes--;
					chown(devname[CHAR],(uid_t)DEV_UID,(gid_t)DEV_GID);
					fprintf(contents_fp,"%s c %d %d ? ? ?\n",devname[RSA],tc_cmajor,minornum);
				}
			}
		}
	} /* end of ParseLine while loop */

	if (!Silent && NodesCreated) {
		printf("\nDevice files have been created for a new %s device:\n", Device_Types[tc_pdtype].string);
		printf("Host Adapter (HA)          = %d\n",sa->sa_c);
		printf("Target Controller (TC) ID  = %d\n",sa->sa_t);
		printf("Logical Unit (LU) ID       = %d\n",sa->sa_l);
		if (postmsg[0] != BLANK) {
			printf("%s", postmsg);
		}
	}

	(void) fclose(templatefp);
}


/*
 * if the machine is booted off SCSI this routine returns TRUE, otherwise
 * it returns FALSE
 */
int
BootedOffSCSI()
{
	int hd_fd;
	short sd01_cmajor, rootbmajor;
	struct drv_majors *majors;
	int majsets;
	int i;
	int root_is_sd01 = FALSE;
	int root_is_mirror = FALSE;
	struct bus_type bus_type;
	STAT	statbuf;

	/* stat /dev/root to get the block major number */
	if (stat("/dev/root",&statbuf) < 0) 
		return(FALSE);

	rootbmajor = major(statbuf.st_rdev);

	if ((majors = GetDriverMajors(SCSIDrivers[SD01].name, &majsets)) == 0) {
		/* sd01 is not configured - not booted from scsi */
		return(FALSE);
	}

	/* get the first character major number */
	sd01_cmajor = majors->c_maj;

	for(i=0; i<majsets; i++) {
		if (rootbmajor == majors->b_maj) {
			root_is_sd01 = TRUE;
			break;
		}
		majors++;
	}

	if(!root_is_sd01) {
		if ((majors = GetDriverMajors(SCSIDrivers[MIRROR].name, &majsets)) == 0) {
			/* mirroring is not configured - not booted from scsi */
			return(FALSE);
		}
		for(i=0; i<majsets; i++) {
			if (rootbmajor == majors->b_maj) {
				root_is_mirror = TRUE;
				break;
			}
			majors++;
		}
	}

	/* if root is not associated with sd01, then return. */
	if((!root_is_sd01) && (!root_is_mirror))
		return(FALSE);

	/* make a raw disk node so that we can check further */
	if (FileExists(TempDev)) unlink(TempDev);
	if (mknod(TempDev, (S_IFCHR|S_IREAD|S_IWRITE), makedev(sd01_cmajor,0)) < 0) {
		warning("%s: mknod failed for temporary node.\n", Cmdname);
		return(FALSE);
	}

	/* try opening the temporary disk node */
	if ((hd_fd = open(TempDev, O_RDONLY)) < 0) {
		warning("%s: open failed for temporary node.\n", Cmdname);
		unlink(TempDev);
		return(FALSE);
	}

	if (ioctl(hd_fd, B_GETTYPE, &bus_type) < 0) {
		close(hd_fd);
		unlink(TempDev);
		return(FALSE);
	}

	close(hd_fd);
	unlink(TempDev);

	if (strncmp (bus_type.bus_name, SCSIDrivers[SCSI].name, 4) != 0) {
		return(FALSE);
	}

	return(TRUE);
}

/* routine to create disk nodes that look like old systems for compatibility.
 * If the disk driver is configured to support 16 subdevices then the nodes are:
 *	/dev/dsk/0s0 through /dev/dsk/0sf
 *	/dev/rdsk/0s0 through /dev/rdsk/0sf
 *	/dev/dsk/1s0 through /dev/dsk/1sf
 *	/dev/rdsk/1s0 through /dev/rdsk/1sf
 */
void
E333A_nodes(subdevs)
int subdevs;
{
	static char obdsk[] = "/dev/dsk/xsx";
	static char ordsk[] = "/dev/rdsk/xsx";
	static char hex[] = "0123456789abcdef";
	int slice, occurrence, minornum;
	short tc_cmajor, tc_bmajor;
	struct drv_majors *majors;
	int majsets;

	minornum=0;

	/*
	 * This code creates compatibility nodes for disk occurrences
	 * 0 and 1 - therefore we are guaranteed that these will use
	 * the first set of major numbers for SD01.
	 */

	if ((majors = GetDriverMajors(SCSIDrivers[SD01].name, &majsets)) == 0) {
		warning("%s: Unable to create compatibility nodes. Can not get major numbers.\n", Cmdname);
		return;
	}
	if (majsets < 1) {
		warning("%s: Unable to create compatibility nodes. Incorrect major number count.\n", Cmdname);
		return;
	}

	/* get the first set of major numbers */
	tc_cmajor = majors->c_maj;
	tc_bmajor = majors->b_maj;

	if (subdevs > 16) subdevs = 16;
	if (subdevs < 0) subdevs = 0;

	for (occurrence=0; occurrence <= 1; occurrence++) {
		obdsk[9] = hex[occurrence];
		ordsk[10] = hex[occurrence];
		for (slice=0; slice < subdevs; slice++) {
			obdsk[11] = hex[slice];
			ordsk[12] = hex[slice];
			/* check for old or incorrect Osx, 1sx nodes
			 * and unlink them.
			 */
			if (!SpecialExists(BLOCK,obdsk,tc_bmajor,minornum)) {
				(void) unlink(obdsk);
				if (mknod(obdsk,DEV_MODE | S_IFBLK,makedev(tc_bmajor,minornum)) < 0) {
					warning("%s: mknod failed for %s.\n", Cmdname,obdsk);
				} else {
					(void)chown(obdsk,(uid_t)DEV_UID,(gid_t)DEV_GID);
					fprintf(contents_fp,"%s b %d %d ? ? ?\n",obdsk,tc_bmajor,minornum);
				}
			}
			if (!SpecialExists(CHAR,ordsk,tc_cmajor,minornum)) {
				(void) unlink(ordsk);
				if (mknod(ordsk,DEV_MODE | S_IFCHR,makedev(tc_cmajor,minornum)) < 0) {
					warning("%s: mknod failed for %s.\n", Cmdname,ordsk);
				} else {
					(void)chown(ordsk,(uid_t)DEV_UID,(gid_t)DEV_GID);
					fprintf(contents_fp,"%s c %d %d ? ? ?\n",ordsk,tc_cmajor,minornum);
				}
			}
			minornum++;
		}
	}
}

void 
str_to_lower(s)
char *s;
{
	while(*s) {
		*s = tolower(*s);
		s++;
	}
}

int
main(argc,argv)
int	argc;
char	**argv;
{
					
	register EDT 	*xedtptr  = NULL;	 /* Pointer to edt */
	register EDT 	*xedtptr2 = NULL;	 /* Temp pointer   */
	register int	c, t, lu;
	struct scsi_addr	scsi_address;
	struct scsi_addr	*sa;
	char		tcinq[INQ_LEN];
	unsigned char	numlus, tc_pdtype;
	int		scsicnt, i, j, arg, force_to_run, ppid;
	int		ignore_old_edt, driver_index;
	int		equiped,can_boot,bootable,unbootable;
	extern char	*optarg;
	char		*s;
	int		no_match;
	STAT		ostatbuf, nstatbuf;
	int		c1, c2;
	int		fd;
	int		num_subdevs;
	char		tempnode[25];
	int		maj;
	int		getdefault = FALSE;
	
/*
 *	Force the command to be pdimkdev if it was run as pdimkdtab
 *		but no arguements were given. Sigh... Can't change it.
 *									Compatability, you know.
 */
	if (argc < 1) {
		(void) strcpy(Cmdname,CMDNAME);
		Pdimkdev = TRUE;
	} else {
		(void) strcpy(Cmdname,basename(argv[0]));
		Pdimkdev = strcmp(Cmdname, CMDNAME) ? FALSE : TRUE;
	}

	/* Open the input file we create for installf */
	if ( Pdimkdev ) {
		if ((contents_fp = fopen(INSTALLF_INPUT,"w")) == NULL)
			error("%s: Could not open %s.\n", Cmdname, TCindex);
	}

	/* set TCindex to default location */
	(void) strcpy(TCindex,TCINDEX);

	Debug = FALSE;
	ignore_old_edt = FALSE;
	force_to_run=FALSE;
	while ((arg = getopt(argc,argv,"Sd:isf")) != EOF)

		switch (arg) {
		case 'd' : /* alternate tc index supplied */
			(void) strcpy(TCindex,optarg);
			break;
		case 's' : /* Silent mode */
			Silent = TRUE;
			break;
		case 'f' : /* force to run regardless of run-level */
			force_to_run = TRUE;
			break;
		case 'S' : /* Turn on debug messages */
			Debug = TRUE;
			break;
		case 'i' : /* ignore configuration saved in previous boot */
			ignore_old_edt = TRUE;
			break;
		case '?' : /* Incorrect argument found */
			before_exit();
			error("usage: %s [-f] [-i] [-d file].\n",Cmdname);
			/*NOTREACHED*/
			break;
		}
	
	/* if the parent is init then run this command */
	ppid = getppid();
	if (ppid == 1) {
		force_to_run = TRUE;
	}

	/* Print error unless forced to run */
	/* or running as pdimkdtab         */
	if (!force_to_run && (strcmp(Cmdname, MKDTABNAME) != 0)) {
		before_exit();
		errno = 0;
		if ((GetDriverMajors(SCSIDrivers[MIRROR].name, &maj)) == 0)
			error("%s: This command is designed to run during the boot sequence.\nDuring the execution of this command all device nodes will be removed.\nUse the -f option to force its execution.\n", Cmdname);
		else
			error("%s: This command is designed to run during the boot sequence.\nDuring the execution of this command all device nodes will be removed.\nUnmirror any mirrored devices beforehand.\nUse the -f option to force its execution.\n", Cmdname);
	}

	/* Ignore certain signals */
if (!Debug) {
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGINT,SIG_IGN);
	(void) signal(SIGTERM,SIG_IGN);
}

	umask(0); /* use template file permission (mode) tokens */

	/* Check to see that the tc.index file exists. */
	if (!FileExists(TCindex)) {
		before_exit();
		error("%s: index file %s does not exist.\n", Cmdname, TCindex);
	}

	/* Open the target controller index file. Exits on failure.  */
	if ((TCindexfp = fopen(TCindex,"r")) == NULL) {
		before_exit();
		error("%s: Could not open %s.\n", Cmdname, TCindex);
	}

	/* Initialize driver info structure */
	for(i = 0; i < MAXTCTYPE; i++) {
		DRVRinfo[i].valid = FALSE;
	}

	/* check to see if this is booted off scsi, before nodes are removed */
	Booted_off_scsi = BootedOffSCSI();

	/*  Make a unique file name for later use  */
	mktemp(TempDev);

	if (((DRVRinfo[0].majors = GetDriverMajors(SCSIDrivers[SDI].name, &DRVRinfo[0].majsets)) != 0) || ((DRVRinfo[0].majors = GetDriverMajors (SCSIDrivers[SCSI].name, &DRVRinfo[0].majsets)) != 0 )) {
		DRVRinfo[0].valid = TRUE;
	}

	if ((xedtptr = readedt(&scsicnt)) == 0) {
		before_exit();
		error("%s: Unable to read equipped device table.\n", Cmdname);
	}

	if (!ignore_old_edt) {
		if (!FileExists(LAST_BOOT_EDT)) 
			ignore_old_edt = TRUE;
		else
			if ((last_edt_fp = fopen(LAST_BOOT_EDT,"r")) == NULL) 
				ignore_old_edt = TRUE;
	}

	if ((temp_edt_fp = fopen(TEMP_EDT,"w+")) == NULL) {
		before_exit();
		error("%s: Could not open %s.\n", Cmdname, TEMP_EDT);
	}
	
	/* write current edt to temp file */
	fwrite (xedtptr, sizeof(struct scsi_edt), MAX_TCS * scsicnt, temp_edt_fp);

	/* compare the size of old and new files. no need to do byte
	 * comparison if the sizes are different */
	if (!ignore_old_edt) {
		rewind(temp_edt_fp);	/* get offset from the file beginning */
		if (stat(TEMP_EDT, &nstatbuf) < 0) {
			before_exit();
			error("%s: Could not stat %s.\n", Cmdname, TEMP_EDT);
		}
		if (stat(LAST_BOOT_EDT, &ostatbuf) < 0) {
			before_exit();
			error("%s: Could not stat %s.\n", Cmdname, LAST_BOOT_EDT);
		}
		if ( ostatbuf.st_size != nstatbuf.st_size )
			ignore_old_edt = TRUE;
	}

	if (!ignore_old_edt) {

		no_match = FALSE;

		/* byte compare previous and current edt */ 
		while ((c1=getc(temp_edt_fp))!= EOF && (c2=getc(last_edt_fp))!= EOF) {
			if (c1 != c2) {
			no_match = TRUE;
			break;
			}
		}

		fclose(temp_edt_fp);
		fclose(last_edt_fp);
		if (!no_match) {
			unlink (TEMP_EDT);
			exit(NO_CHANGES_EXIT);
		}
	}
	if (ignore_old_edt)
		fclose(temp_edt_fp);
	rename(TEMP_EDT, LAST_BOOT_EDT);
	chmod(LAST_BOOT_EDT, MASK);

	/* convert all the drv_name fields to lower case */
	xedtptr2 = xedtptr;
	for (c = 0; c < scsicnt; ++c) {
		for (t = 0; t < MAX_TCS; t++, xedtptr2++) {
			str_to_lower(xedtptr2->drv_name);
		}
	}

	/* set the pdtype field of host adapter entries */
	xedtptr2 = xedtptr;
	for (c = 0; c < scsicnt; ++c) {
		for (t = 0; t < MAX_TCS; t++, xedtptr2++) {
			if ((strcmp((char *) xedtptr2->drv_name,SCSIDrivers[SDI].name) == 0) || (strcmp((char *) xedtptr2->drv_name, SCSIDrivers[SCSI].name) == 0)) {
				xedtptr2->pdtype = HOST_ADAPTER;
			}
		}
	}

	/* mark entries unequipped if drv_name is NULL or "void" */
	xedtptr2 = xedtptr;
	for (c = 0; c < scsicnt; ++c) {
		for (t = 0; t < MAX_TCS; t++, xedtptr2++) {

			if (xedtptr2->tc_equip == 0) {
			        continue;
			}

			if (xedtptr2->drv_name == NULL) {
				xedtptr2->tc_equip = 0;
			        continue;
			}

			if (strcmp((char *) xedtptr2->drv_name,"void") == 0) {
				xedtptr2->tc_equip = 0;
			        continue;
			}
		}
	}


	/* initialize the scsi address pointer */
	sa = &scsi_address;
	
	/* 
	 * Add the mirror major numbers to the DRVRinfo
	 * array for use in the next step
	 */
	if ((DRVRinfo[1].majors = GetDriverMajors(SCSIDrivers[MIRROR].name, &DRVRinfo[1].majsets)) != 0) {
		DRVRinfo[1].valid = TRUE;
	}

	/* 
	 * Walk through the EDT and check for devices in the
	 * previous configuration. Consider any device that
	 * has it's first device file created, as being in
	 * the previous configuration.
	 */
	for ( xedtptr2 = xedtptr, bootable=-1, unbootable=0, c = 0; c < scsicnt; ++c) {
		HBA[c].edtptr = xedtptr2;
		for (can_boot=FALSE, equiped=FALSE, t=0; t < MAX_TCS; t++, xedtptr2++) {

			if (xedtptr2->tc_equip == 0) {
			    continue;
			}

			equiped = TRUE;

			tc_pdtype = xedtptr2->pdtype; 
			
			if (tc_pdtype == ID_RANDOM || tc_pdtype == ID_ROM || tc_pdtype == ID_WORM) {
				can_boot = TRUE;
			}

			/* The following was added to support the minor 
			 * number indexing scheme on 386. The lu_occur field
	 		 * is used to count the occurrence of lu's of a 
			 * particular type. The first LU of the first TC of a 
			 * particular tctype will be occurrence zero.
			 * This value can be obtained in the 
			 * template file by specifying the letter 'D' in the 
			 * minor number calculation. 
			 */ 
			for (driver_index = -1, i = 0; i < MAXTCTYPE; i++) {
				if (DRVRinfo[i].valid == FALSE) {
					DRVRinfo[i].valid = TRUE;
					strcpy(DRVRinfo[i].name, xedtptr2->drv_name);
					DRVRinfo[i].majors =
						GetDriverMajors(DRVRinfo[i].name,&DRVRinfo[i].majsets);
					if (DRVRinfo[i].majors == 0) {
						before_exit();
						errno = 0;
						error("%s: Driver name (%s) appears in EDT, but not in mdevice file.\n", Cmdname, xedtptr2->drv_name);
					}
				/*
				 *	get the default values of Scsi Subdevices for
				 *	each target occurence from the SCSIDrivers array.
				 */
					num_subdevs = SCSI_SUBDEVS;
					for (j = 0; j < NUMDRVRS; j++) {
						if (!strcmp(SCSIDrivers[j].name, DRVRinfo[i].name)) {
							num_subdevs = SCSIDrivers[j].subdev;
							break;
						}
					}

					DRVRinfo[i].subdevs  = num_subdevs;
					DRVRinfo[i].lu_limit = (MAXMINOR + 1)/num_subdevs;
					DRVRinfo[i].lu_occur = -1;
					DRVRinfo[i].lu_majors= DRVRinfo[i].majors;
					DRVRinfo[i].admin_count = 1;
					driver_index = i;
					break;
				} else if (strcmp(DRVRinfo[i].name, xedtptr2->drv_name) ==  0) {
					/* found it */
					driver_index = i;
					break;
				}
				/* else keep looking */
			}

			if (driver_index < 0) {
				before_exit();
				errno = 0;
				error("%s: Too many drivers. Only %d drivers supported.\n", Cmdname, MAXTCTYPE);
			}
		} /* end TC loop */

		if (can_boot == TRUE) {
			HBA[c].index = ++bootable;
		} else {
			if ( equiped == TRUE ) {
				HBA[c].index = --unbootable;
			}
		}
	} /* end HA loop */
/*
 *	HBA is an array used to record information about each controller.
 *	The best place for this information would be in the edt but thats
 *	too tricky to modify on the fly.
 *
 *	HBA.index contains a numbering of the controllers in the HBA that
 *	is dependent on the bootability of a given HBA.  This is used to
 *	designate the controllers in such a way that c0 is always the
 *	boot controller.  c1 is the next logical controller.  Devices that
 *	can boot ( ie have disk ) come before HBAs that don't have disk.
 *
 *	HBA.order contains an index into the HBA that
 *	allows a reference to HBA in the order of increasing index.
 *
 *	HBA.edtptr points to the start of the edt array for each controller
 *	since now the nodes for the first controller in the edt may not be called
 *	c0 anymore.  We need to start at the right place in the EDT so
 *	things get made and numbered in the right order.
 */

	for (c = 0; c < scsicnt; c++) {
		if (HBA[c].index < 0) {
			HBA[c].index = bootable - HBA[c].index;
		}
		HBA[HBA[c].index].order = c;
	}

	/* Count the number of free inodes available on the root file system */
	Freeinodes = NumFreeInodes();

	MakeSCSIMajors();

	if ( Pdimkdev ) {
		/* Delete all the SCSI nodes in the /dev subdirectories */
		if (FileExists("/dev/scsi") && RemoveSCSINodes("/dev/scsi") < 0 ) {
			warning("%s: Unable to remove old SCSI nodes in /dev/scsi\n", Cmdname);
		}
	} else
		ClearDeviceTable();

	if (Debug) {
		printf("driver\tha_slot\tTC\tnumlus\tPDtype\tequip\tTCinq\n");
	}

	for ( c = 0; c < scsicnt; c++ ) {

		/*
		 * The structure sa is used to pass the current values
		 *	of C, T, L and N out of this loop into MakeDeviceNodes
		 *
		 * We need to actually number things from 0 as we start
		 *	here.  So the controller number used to name all nodes
		 *	we make or link in this loop is simply incremented.
		 */

		sa->sa_c = c;

		/*
		 * now start in the order we want to really make the devices in.
		 *
		 * The device that we are making nodes for, as described by
		 *	the array of edt entries, may not be the first device in the
		 *	edt ( Equipped Device Table ).  This is caused by constraints
		 *	on ordering the HBA table imposed by the Loadable Driver stuff.
		 *
		 * So, lets set our starting point for each controller.  We need
		 *	a pointer into the edt that reflects actual position in the edt.
		 *
		 * Now lets make devices for all the targets and luns used by
		 *	the edt_order'th controller in the HBA table.
		 *
		 */

		xedtptr2 = HBA[HBA[c].order].edtptr;

		for (t = 0; t < MAX_TCS; t++, xedtptr2++) {

			sa->sa_t = t;

			strcpy(tcinq,(char *) xedtptr2->tc_inquiry);
			tc_pdtype = xedtptr2->pdtype;
			numlus    = xedtptr2->n_lus;

			if (Debug) {
			printf("%s\t%d\t%d\t%d\t%d\t%d\t%s\n",
				xedtptr2->drv_name,
				xedtptr2->ha_slot,
				t,
				xedtptr2->n_lus,
				xedtptr2->pdtype,
				xedtptr2->tc_equip,
				xedtptr2->tc_inquiry);
			}
			if (xedtptr2->tc_equip == 0)
				continue;

			/* increment the occurrence field for the driver */
			for(i = 0; i < MAXTCTYPE; i++) {
				if(DRVRinfo[i].valid == TRUE) {
					if (strcmp(DRVRinfo[i].name, xedtptr2->drv_name) !=  0) 
						continue;

					DRVRinfo[i].lu_occur += 1;
					if (DRVRinfo[i].lu_occur >= DRVRinfo[i].lu_limit) {
						DRVRinfo[i].lu_occur = 0;
						DRVRinfo[i].lu_majors++;
					}
					break;
				}
				else if (DRVRinfo[i].valid == FALSE) {
					before_exit();
					errno = 0;
					error("%s: Problem transversing EDT.\n", Cmdname);
				}
				/* else keep looking */
			}
			if ((driver_index = i) == MAXTCTYPE) {
				before_exit();
				errno = 0;
				error("%s: Too many drivers. Only %d drivers supported.\n", Cmdname, MAXTCTYPE);
			}

			for(lu = 0; lu < MAX_LUS; lu++) {
			    if(xedtptr2->lu_id[lu]) {

					sa->sa_l = lu;
					sa->sa_n = DRVRinfo[driver_index].admin_count++;

					MakeDeviceNodes(sa,driver_index,tcinq, tc_pdtype,
									BUS_TYPE(xedtptr2->ha_slot));
					if(--numlus) {
						DRVRinfo[driver_index].lu_occur += 1;
						if 	( DRVRinfo[driver_index].lu_occur >= \
							  DRVRinfo[driver_index].lu_limit) {
							DRVRinfo[driver_index].lu_occur = 0;
							DRVRinfo[driver_index].lu_majors++;
						}
					}
				}
			} /* end LU loop */
		} /* end TC loop */
	} /* end HA loop using boot-chain based index into EDT */

	(void) fclose (TCindexfp);

	/* make compatibility nodes and update the contents file */
	/* before_exit does nothing if this is mkdtab */
	before_exit();

	exit(NORMEXIT);
	/* NOTREACHED */
}


/* OA&M population utility code starts here */

#define BLKSIZE	512

#define SCSI_ATTR	"scsi=true"
#define ATTR_TERM	(char *)NULL

#define GOODEXIT	0
#define BADEXIT		-1

typedef struct type_value
{
	char	type[MAXLINE];
	int	N;
	struct type_value *next;
} TYPE_VALUE_T;
struct type_value	*list;

static int		vfsnum;
static struct vfstab	*vfstab;

extern int    errno;
extern void   free();
extern char  *malloc();

extern int  ClearDeviceTable();
extern int  MakeDeviceTable();
static int  getvar();
static int  getn();
static void expand();
static void add_dpart();
static int  initialize();
static int  s5part();
static char *memstr();

/*
 *  ClearDeviceTable
 *       removes all scsi device entries from the device table
 *       removes scsi device entries from the scsi device groups
 *            but not from administrator defined groups
 *
 *       returns 0 on success, -1 on failure
 */
extern int
ClearDeviceTable()
{
	char	**criteria_list;
	char	**criteria_ptr;
	char	**rm_list;	/* devices with scsi=true to be removed */
	char	**dev_ptr;
	char	*type;
	char	criteria[MAXLINE];
	char	**dgrp_list;
	char	**dgrp_ptr;
	char	system_str[2*MAXLINE];

	/* initialize list to empty (list used in getn()) */
	list = (struct type_value *)NULL;

	/* get list of devices with attribute scsi=true */
	/* build criteria list */
	criteria_list = (char **) malloc(3*sizeof(char **));
	criteria_ptr = criteria_list;
	*criteria_ptr++ = SCSI_ATTR;
	*criteria_ptr = ATTR_TERM;

	if ( (rm_list = getdev((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
	{
		warning("Unable to clear device table %s, getdev failure.\n", DTAB_PATH);
		return(BADEXIT);
	}

	for (dev_ptr = rm_list; *dev_ptr != (char *)NULL; dev_ptr++)
	{
		 /* find scsi device group that this device is in */
		 /* first find out which type scsi device we are removing */
		if ( (type = devattr(*dev_ptr, "type")) == (char *)NULL)
		{
			warning("Unable to clear device table %s, devattr failure.\n", DTAB_PATH);
			continue;
		}
		/* then get a list of groups with that device type */
		(void) sprintf(criteria, "type=%s", type);
		criteria_ptr = criteria_list + 1;  /* skip over scsi=true criteria */
		*criteria_ptr++ = criteria;
		*criteria_ptr = ATTR_TERM;
		if ( (dgrp_list = getdgrp((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
		{
			/*
			 * This will only occur if an error occurs during getdgrp.
			 * If no groups are present meeting the criteria, a list
			 * with the first element a NULL pointer is returned.
			 * This is true for all device table functions.
			 */
			warning("Unable to clear device group table %s, getdgrp failure.\n", DGRP_PATH);
			continue;
		}

		/* remove the device from the SCSI device groups */
		for (dgrp_ptr = dgrp_list; *dgrp_ptr != (char *)NULL; dgrp_ptr++)
		{
			if (strncmp(*dgrp_ptr, SCSIDrivers[SCSI].name, 4) != 0)
				continue;
			/*  attempt to remove device from group
			 *  ignore return code since we didn't
			 *       check that device is a member of dgrp
			 */
			(void) sprintf(system_str, "putdgrp -d %s %s > /dev/null 2>&1", *dgrp_ptr, *dev_ptr);
			(void) system(system_str);
		}
		free((char *)dgrp_list);

		/* remove device from device table */
		(void) sprintf(system_str, "putdev -d %s > /dev/null 2>&1", *dev_ptr);
		(void) system(system_str);
	}
	free((char *)criteria_list);
	free((char *)rm_list);

	/* these lines are a work-around for a bug in libadm
	 * close the device and device group tables so that
	 * the above removals will be written to the tables
	 */
	_enddevtab();
	_enddgrptab();

	/* initialize copy of vfstab in memory for use in adding dpart entries */
	return(initialize());
}

/*
 *  MakeDeviceTable
 *       reads the template file to get the alias, device group
 *            and attribute list
 *       create a unique alias for the device table using the alias
 *            for example, the alias is disk and disk1, disk2 exist
 *            then the alias for this device should be disk3
 *       translate variables in the attribute list using the info
 *            passed in
 *            for example, an attribute may be prtvtoc $CDEVICE$ so
 *            substitute the character device passed in for CDEVICE
 *       add the new device to the device table
 *       add the new device to the device group
 *       if type is disk, add partition entries
 *
 *       returns 0 on success, -1 on failure
 */
extern int
MakeDeviceTable(b_dev_name, c_dev_name, tc_inquiry, bus_version, tc_pdtype)
char	*b_dev_name;
char	*c_dev_name;
char	*tc_inquiry;
int	bus_version;
unsigned char	tc_pdtype;
{
	char	alias[MAXLINE], dgrp[MAXLINE], attr[2*MAXLINE], fsattr[2*MAXLINE], dpattr[2*MAXLINE];
	int	n;
	char	n_str[5];
	char	exp_alias[MAXLINE];
	char	bus_str[MAXLINE];
	char	exp_attr[2*MAXLINE];
	char	system_str[2*MAXLINE];
	char	dpartlist[MAXLINE];

	(void) sprintf(alias, "");
	(void) sprintf(dgrp, "");
	(void) sprintf(attr, "");
	(void) sprintf(fsattr, "");
	(void) sprintf(dpattr, "");
	(void) sprintf(dpartlist, "");
	
	/* retrieve alias, device group and attribute list */
	if (getvar(tc_inquiry, alias, dgrp, attr, fsattr, dpattr, tc_pdtype) == BADEXIT)
	{
		/* set errno to zero so that warning will not use perror */
		errno = 0;
		warning("Unable to retrieve alias, device group and attribute list\n\tfrom template file specified by tc_inquiry string %s.\nNo device entry added to device table %s\n\tfor character device %s.\n", tc_inquiry, DTAB_PATH, c_dev_name);
		return(BADEXIT);
	}

	/* get number value necessary to create unique alias */
	n = getn(alias);
	(void) sprintf(n_str, "%d", n);
	(void) sprintf(exp_alias, "%s%d", alias, n);

	/* expand variables in attribute list to character or block device
	 * passed in
	 */
	expand(b_dev_name, c_dev_name, n_str, attr, exp_attr);

	/* add bus attribute to attribute list */
	(void) sprintf(bus_str, " scsibus=%d", bus_version);
	(void) strcat(attr, bus_str);

	/* add new device to device table */
	/* need more than one form of command because the block device
	 * name may be null and putdev no longer accepts null parameters
	 */
	if (b_dev_name == (char *)NULL || strlen(b_dev_name) == 0)
		(void) sprintf(system_str, "putdev -a %s %s=%s %s", exp_alias, DTAB_CDEVICE, c_dev_name, exp_attr);
	else
		(void) sprintf(system_str, "putdev -a %s %s=%s %s=%s %s", exp_alias, DTAB_BDEVICE, b_dev_name, DTAB_CDEVICE, c_dev_name, exp_attr);
	(void) system(system_str);

	/* add new device to device group table */
	(void) sprintf(system_str, "putdgrp %s %s", dgrp, exp_alias);
	(void) system(system_str);

	if (strcmp(alias, "disk") == 0)
	{
		/* add entries for partitions */
		if (strlen(fsattr) == 0 || strlen(dpattr) == 0)
		{
			errno = 0;
			warning("FSATTR or DPATTR tokens not defined in template file\n\treferenced by %s inquiry string.\nNo partition entries added to device table for %s.\n", tc_inquiry, exp_alias);
			return(GOODEXIT);
		}
		add_dpart(b_dev_name, c_dev_name, n, fsattr, dpattr, dpartlist);
		if (strlen(dpartlist) == 0)
			return(GOODEXIT);
		(void) sprintf(system_str, "putdev -m %s dpartlist=%s", exp_alias, dpartlist);
		(void) system(system_str);
	}
		
	return(GOODEXIT);
}

/*
 * getvar
 *
 * open template file using tc_inquiry string
 * read template file searching for the ALIAS, DGRP, ATTR and possibly
 *      FSATTR and DPATTR tokens
 * close template file
 */
static int
getvar(tc_inquiry, alias, dgrp, attr, fsattr, dpattr, tc_pdtype)
char	*tc_inquiry;
char	*alias;
char	*dgrp;
char	*attr;
char	*fsattr;
char	*dpattr;
unsigned char	tc_pdtype;
{
	FILE		*templatefp;
	int		data_begin;
	register int	tokenindex;

	if ((templatefp = OpenTemplate(tc_pdtype, tc_inquiry)) == NULL)
		return(BADEXIT);

	data_begin = 0;
	while (!data_begin)
	{
		tokenindex = GetToken(templatefp);
		switch(tokenindex)
		{
			case ALIAS:
				(void) fscanf(templatefp, " %[^\n]\n", alias);
				break;
			case DGRP:
				(void) fscanf(templatefp, " %[^\n]\n", dgrp);
				break;
			case ATTR:
				(void) fscanf(templatefp, " %[^\n]\n", attr);
				break;
			case FSATTR:
				(void) fscanf(templatefp, " %[^\n]\n", fsattr);
				break;
			case DPATTR:
				(void) fscanf(templatefp, " %[^\n]\n", dpattr);
				break;
			case DATA:
				data_begin = 1;
				break;
			case EOF:
				(void) fclose(templatefp);
				return(BADEXIT);
				/* NOTREACHED */
				break;
			default:
				(void) fscanf(templatefp, "%*[^\n]%*[\n]");
				break;
		}
	}
	(void) fclose(templatefp);
	if (strlen(alias) == 0 || strlen(dgrp) == 0 || strlen(attr) == 0)
		return(BADEXIT);
	return(GOODEXIT);
}

/*
 *  getn
 *
 *  get entries already in device table of this type
 *  find the current highest value of N in typeN
 *  add 1 to N and return for use in new alias
 */
static int
getn(alias)
char	*alias;
{
	struct type_value	*ptr;
	struct type_value	*lastptr;
	char			criteria[MAXLINE];
	char			**criteria_list;
	char			**criteria_ptr;
	char			**dev_list;
	char			**dev_ptr;
	int 			n, id;

	/* has this type been evaluated before */
	for (ptr = list, lastptr = list; ptr != (struct type_value *)NULL; ptr = ptr->next)
	{
		/* save value for end of list in case another member must be added */
		if (ptr->next != (struct type_value *)NULL)
			lastptr = ptr->next;
		if (strcmp(alias, ptr->type) == 0)
			break;
	}

	if (ptr != (struct type_value *)NULL)
	{
		/* N value for this type has been found from table already */
		n = ptr->N;
		ptr->N = n + 1;
		return(n);
	}

	/* build criteria list, criteria is type = alias from template */
	criteria_list = (char **) malloc(2*sizeof(char **));
	criteria_ptr = criteria_list;
	(void) sprintf(criteria, "type=%s", alias);
	*criteria_ptr++ = criteria;
	*criteria_ptr = ATTR_TERM;

	/*
	 * Note that getdev returns (char **)NULL only if an error occurs.
	 * If no devices exist meeting the criteria, getdev returns a
	 * dev_list where the first element is a (char *)NULL
	 */
	if ( (dev_list = getdev((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
	{
		warning("Unable to get current list of devices of type %s.\nUsing %s0 as alias.\n", alias, alias);
		free((char *)criteria_list);
		return(0);
	}

	free((char *)criteria_list);

	/* get the highest N value from typeN */
	n = id = 0;
	for (dev_ptr = dev_list; *dev_ptr != (char *)NULL; dev_ptr++)
	{
		id = atoi(*dev_ptr + strlen(alias));
		if (id > n)
			n = id;
	}
	n = n + 1;
	free((char *)dev_list);

	/* if the alias is disk and 
	 * if we are not Booted_off_scsi, we are booted off ESDI
	 * start n at at least 3 to allow for 2 ESDI disks in the future
	 */
	if ( (strcmp(alias, "disk") == 0) && (Booted_off_scsi == FALSE) && (n < 3) )
		n = 3;

	/* add this type to the list */
	ptr = (struct type_value *)malloc(sizeof(struct type_value));
	(void) strcpy(ptr->type, alias);
	ptr->N = n + 1;
	ptr->next = (struct type_value *)NULL;
	if (lastptr == (struct type_value *)NULL)
		/* first member of list */
		list = ptr;
	else
		lastptr->next = ptr;

	/* return N for use in creating alias */
	return(n);
}

/*
 *  expand
 * 
 *  variables in attribute list are surrounded by $ characters
 *  find $ character and translate next token
 */
static void
expand(b_dev_name, c_dev_name, n_str, attr, exp_attr)
char	*b_dev_name;
char	*c_dev_name;
char	*n_str;
char	*attr;
char	*exp_attr;
{
	char	*attr_var;
	char	*dev_name;
	char	*dev;

	/* dev_name is device name without path (cNtNdNsN)
	 * dev is device name without path or slice (cNtNdN)
	 * create device name from character device name
	 * this assumes that device name is in form /dev/rxxx/device_name
 	 * or something similar
	 * strip the sN for dev
 	 */
	dev_name = strrchr(c_dev_name, '/') + 1;
	dev = strdup(dev_name);
	(void)strtok(dev, "s");

	attr_var = strtok(attr, "$");
	(void) strcpy(exp_attr, attr);
	while (attr_var != (char *)NULL)
	{
		attr_var = strtok((char *)NULL, "$");
		if (attr_var == NULL)
			continue;
		/* N value */
		if (strcmp(attr_var, "N") == 0)
		{
			(void) strcat(exp_attr, n_str);
		}
		/* block device name */
		else if (strcmp(attr_var, "BDEVICE") == 0)
		{
			(void) strcat(exp_attr, b_dev_name);
		}
		/* character device name */
		else if (strcmp(attr_var, "CDEVICE") == 0)
		{
			(void) strcat(exp_attr, c_dev_name);
		}
		/* device name without character or block path */
		else if (strcmp(attr_var, "DEVICE") == 0)
		{
			(void) strcat(exp_attr, dev_name);
		}
		else if (strcmp(attr_var, "DEV") == 0)
		{
			(void) strcat(exp_attr, dev);
		}
		else
		{
			errno = 0;
			warning("Cannot expand variable %s.\n", attr_var);
		}

		attr_var = strtok((char *)NULL, "$");
		if (attr_var != NULL)
			(void) strcat(exp_attr, attr_var);
	}
	return;
}

/*
 * add_dpart() gets information about the specified hard drive from the vtoc
 * and vfstab and adds the partition entries to device.tab.
 */
static void
add_dpart(b_dev_name, c_dev_name, n, fsattr, dpattr, dpartlist)
char	*b_dev_name;
char	*c_dev_name;
int	n;
char	*fsattr;
char	*dpattr;
char	*dpartlist;
{
	struct vtoc	vtoc;
	int		s, j;
	static char	hex[] = "0123456789abcdef";
	char		hex_s;
	char		ns_str[5];
	char		exp_alias[MAXLINE], exp_bdevice[MAXLINE], exp_cdevice[MAXLINE], exp_attr[2*MAXLINE];
	char		tmpfsattr[2*MAXLINE], tmpdpattr[2*MAXLINE];
	char		system_str[2*MAXLINE];
	char		*mountpoint;
	int		first = 0;

	if (rd_vtoc(c_dev_name, &vtoc) <= 0)
		return;

	/*
	 * Build a table of disk partitions we are interested in and finish
	 * add each partition to the dpartlist.
	 */
	for (s = 0; s < (int)vtoc.v_nparts; ++s)
	{
		if (vtoc.v_part[s].p_size == 0 || ((vtoc.v_part[s].p_flag & V_VALID) != V_VALID))
			continue;
		hex_s = hex[s];
		(void) sprintf(exp_cdevice, "%.*s%c", strlen(c_dev_name) - 1, c_dev_name, hex_s);
		if (strcmp(exp_cdevice, c_dev_name) == 0)
			/* don't add a dpart entry for partition that has
			 * been added as a disk entry
			 */
			continue;
		(void) sprintf(exp_bdevice, "%.*s%c", strlen(b_dev_name) - 1, b_dev_name, hex_s);
		(void) sprintf(ns_str, "%d%c", n, hex_s);
		(void) sprintf(exp_alias, "dpart%s", ns_str);

		/* expand destroys the attribute list so copy lists
		 * to temporary lists for use by next partition
		 */
		(void) strcpy(tmpfsattr, fsattr);
		(void) strcpy(tmpdpattr, dpattr);
		expand(exp_bdevice, exp_cdevice, ns_str, tmpfsattr, exp_attr);

		/*
		 * We assemble the rest of the information about the partitions by
		 * looking in the vfstab and at the disk itself.  If vfstab says the
		 * partition contains a non-s5 file system we believe it, otherwise
		 * we call s5part() which will check for an s5 super block on the disk
		 * and do the putdev if found
		 */
		for (j = 0; j < vfsnum; j++)
		{
			if(strcmp(exp_bdevice,vfstab[j].vfs_special)==0)
				break;
		}
		if (j < vfsnum)
		{
			/*
			 * Partition found in vfstab.
			 */
			if (strncmp(vfstab[j].vfs_fstype,"s5",2) == 0)
			{
				/*
				 * Call s5part() but ignore return value. If
				 * s5part() finds a file system it will create
				 * the device.tab entry.  If not, we have a
				 * conflict with what vfstab says so we leave
				 * this partition out of device.tab.
				 */
				(void) s5part(exp_alias, exp_bdevice, exp_cdevice, exp_attr, vtoc.v_part[s].p_size, vfstab[j].vfs_mountp);
			}
			else
			{
				if (strcmp(vfstab[j].vfs_mountp, "-") == 0)
					mountpoint="/mnt";
				else
					mountpoint=vfstab[j].vfs_mountp;
				/* add new device to device table */
				(void) sprintf(system_str, "putdev -a %s %s=%s %s=%s capacity=%d mountpt=%s %s", exp_alias, DTAB_BDEVICE, exp_bdevice, DTAB_CDEVICE, exp_cdevice, vtoc.v_part[s].p_size, mountpoint, exp_attr);
				(void) system(system_str);
			}
		}
		else
		{
			/*
			 * Partition not in vfstab.  See if it's an s5
			 * file system; if not, call it a data partition.
			 */
			if (s5part(exp_alias, exp_bdevice, exp_cdevice, exp_attr, vtoc.v_part[s].p_size, (char *)NULL) == BADEXIT)
			{
				/* add new device to device table */
				expand(exp_bdevice, exp_cdevice, ns_str, tmpdpattr, exp_attr);
				(void) sprintf(system_str, "putdev -a %s %s=%s %s=%s capacity=%d %s", exp_alias, DTAB_BDEVICE, exp_bdevice, DTAB_CDEVICE, exp_cdevice, vtoc.v_part[s].p_size, exp_attr);
				(void) system(system_str);
			}
		}
		/* add new device to device group table */
		(void) sprintf(system_str, "putdgrp scsidpart %s", exp_alias);
		(void) system(system_str);

		/* add alias to dpartlist to be returned */
		if (first == 0)
		{
			(void) sprintf(dpartlist, "%s", exp_alias);
			first++;
		}
		else
		{
			(void) strcat(dpartlist, ",");
			(void) strcat(dpartlist, exp_alias);
		}
	}
	return;
}


static int
initialize()
{
	FILE		*fp;
	int		i;
	struct vfstab	vfsent;

	/*
	 * Build a copy of vfstab in memory for later use.
	 */
	if ((fp = fopen("/etc/vfstab", "r")) == NULL)
	{
		warning("No device entries will be added to device table %s for disk partitions.\n\tUnable to open /etc/vfstab.\n", DTAB_PATH);
		return(BADEXIT);
	}

	/*
	 * Go through the vfstab file once to get the number of entries so
	 * we can allocate the right amount of contiguous memory.
	 */
	vfsnum = 0;
	while (getvfsent(fp, &vfsent) == 0)
		vfsnum++;
	rewind(fp);

	if ((vfstab = (struct vfstab *)malloc(vfsnum * sizeof(struct vfstab))) == NULL)
	{
		warning("No device entries will be added to device table %s for disk partitions.\n\tOut of memory for /etc/vfstab.\n", DTAB_PATH);
		return(BADEXIT);
	}

	/*
	 * Go through the vfstab file one more time to populate our copy in
	 * memory.  We only populate the fields we'll need.
	 */
	i = 0;
	while (getvfsent(fp, &vfsent) == 0 && i < vfsnum)
	{
		if (vfsent.vfs_special == NULL)
			vfstab[i].vfs_special = NULL;
		else
			vfstab[i].vfs_special = memstr(vfsent.vfs_special);
		if (vfsent.vfs_mountp == NULL)
			vfstab[i].vfs_mountp = NULL;
		else
			vfstab[i].vfs_mountp = memstr(vfsent.vfs_mountp);
		if (vfsent.vfs_fstype == NULL)
			vfstab[i].vfs_fstype = NULL;
		else
			vfstab[i].vfs_fstype = memstr(vfsent.vfs_fstype);
		i++;
	}
	(void)fclose(fp);
	return(GOODEXIT);
}


/*
 * s5part() reads the raw partition looking for an s5 file system.  If
 * it finds one it adds a partition entry to device.tab using the
 * information passed as arguments and additional info read from the
 * super-block.
 */
static int
s5part(exp_alias, exp_bdevice, exp_cdevice, exp_attr, capacity, mountpt)
char	*exp_alias;
char	*exp_bdevice;
char	*exp_cdevice;
char	*exp_attr;
long	capacity;
char	*mountpt;
{
	int		fd;
	long		lbsize, ninodes;
	struct filsys	s5super;
	char		*mountpoint;
	struct stat	psb, rsb;
	char	system_str[2*MAXLINE];

	if ((fd = open(exp_cdevice, O_RDONLY)) == -1)
		return(BADEXIT);

	if (lseek(fd, SUPERBOFF, SEEK_SET) == -1)
	{
		(void)close(fd);
		return(BADEXIT);
	}

	if (read(fd, &s5super, sizeof(struct filsys)) < sizeof(struct filsys))
	{
		(void)close(fd);
		return(BADEXIT);
	}

	(void)close(fd);

	if (s5super.s_magic != FsMAGIC)
		return(BADEXIT);

	switch(s5super.s_type)
	{
		case Fs1b:
			lbsize = 512;
			ninodes = (s5super.s_isize - 2) * 8;
			break;
		case Fs2b:
			lbsize = 1024;	/* may be wrong for 3b15 */
			ninodes = (s5super.s_isize - 2) * 16; /* may be wrong for 3b15*/
			break;
		case Fs4b:
			lbsize = 2048;
			ninodes = (s5super.s_isize - 2) * 32;
			break;
		default:
			return(BADEXIT);
	}

	if (mountpt != NULL)
	{
		mountpoint = mountpt;	/* Use mount point passed as arg */
	}
	else
	{
		if (strcmp(s5super.s_fname, "root") == 0 &&
		    stat(exp_bdevice, &psb) == 0 &&
		    stat("/dev/root", &rsb) == 0 &&
		    psb.st_rdev ==  rsb.st_rdev)
			mountpoint = "/";
		else
			mountpoint = "/mnt";
	}
	(void) sprintf(system_str, "putdev -a %s %s=%s %s=%s capacity=%d fstype=s5 mountpt=%s fsname=%s volname=%s lbsize=%d nlblocks=%d ninodes=%d %s", exp_alias, DTAB_BDEVICE, exp_bdevice, DTAB_CDEVICE, exp_cdevice, capacity, mountpoint, s5super.s_fname, s5super.s_fpack, lbsize, s5super.s_fsize, ninodes, exp_attr);
	(void) system(system_str);
	return(GOODEXIT);
}

static char *
memstr(str)
register char	*str;
{
	register char	*mem;

	if ((mem = (char *)malloc((unsigned)strlen(str) + 1)) == NULL)
	{
		warning("No device entries will be added to device table %s for disk partitions.\n\tOut of memory.\n", DTAB_PATH);
		return((char *)NULL);
	}
	return(strcpy(mem, str));
}
