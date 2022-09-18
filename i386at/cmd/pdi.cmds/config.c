/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:config.c	1.11"
#ident  "$Header: miked 5/26/92$"

/*
 * pdiconfig [-aS] [-f driver] [-R root] [outputfile]
 *
 * /etc/scsi/pdiconfig is a utility that establishes the existence of
 * equipped PDI devices and outputs the data neccessary to run diskcfg.
 * It determines the device equippage by examining the edt.
 * Using its arguments, it can be forced to output information
 * that will cause the configuration of devices that do not exist.
 * This is useful for forcing the inclusion of drivers for hardware
 * that is about to be installed.
 *
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<string.h>
#include	<limits.h>
#include	<dirent.h>
#include	<nlist.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<sys/signal.h>
#include	<sys/vtoc.h>

#include	<sys/scsi.h>
#include	<sys/sdi_edt.h>
#include	<sys/sdi.h>
#include	<sys/scsicomm.h>

#include	"diskcfg.h"

/* Type Definitions */
typedef	struct	scsi_edt	EDT;

extern int	errno,
			opterr,
			optind;
extern char	*optarg;

#define MAX_FORCE 50

static char	*Cmdname,
			*root,
			*force[MAX_FORCE],	/* pointers to short names of devices to force in */
			format[50];

int	Debug,
	err_flag,
	force_flag,
	scsicnt,
	all_flag,
	adapter_count;
/*
 *	This array should be two arrays but, since it works like this,
 *	I am not going to change it just now.  There should be two arrays
 *	with the first containing index and edtptr and the second
 *	containing just index.  It would be less confusing to understand.
 */

struct HBA {
	int	index;		/* controller number of the c'th HBA */
	int	order;		/* translation from HBA order to device order */
	EDT	*edtptr;	 			/* ptr to HBA[c]'s EDT starting point */
} HBA[MAX_HAS];

#define KERNEL "/stand/unix"

/*
 *	The algorithm for determining bootable in reorder_config below
 *	is identical to code in pdimkdev ( mkdev.c ).  Both pdiconfig
 *	and pdimkdev should use a common subroutine to do this to make
 *	sure their views of the PDI configuration always agree.
 *	Again, it ain't broke, don't fix it. ( yet )
 */
static void
reorder_config(xedtptr,scsicnt)
EDT *xedtptr;
int	scsicnt;
{
	EDT *xedtptr2;
	int	bootable, unbootable, c, t, can_boot, equiped;

	/* 
	 * Walk through the EDT and determine the order of bootable
	 * and non-bootable devices.
	 */
	for ( xedtptr2 = xedtptr, bootable=-1, unbootable=0, c = 0; c < scsicnt; ++c) {
		HBA[c].edtptr = xedtptr2;

		for (can_boot=FALSE, equiped=FALSE, t=0; t < MAX_TCS; t++, xedtptr2++) {

			if (xedtptr2->tc_equip == 0 || xedtptr2->pdtype == ID_PROCESOR ) {
			    continue;
			}

			equiped = TRUE;

			if (xedtptr2->pdtype == ID_RANDOM ||
				xedtptr2->pdtype == ID_ROM ||
				xedtptr2->pdtype == ID_WORM) {
					can_boot = TRUE;
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
 *	There is no real need to use bootable and non-bootable here.
 *	It would be simpler to set index to -1 instead of --unbootable
 *	above and then, to set the negative entries to bootable++ below.
 *	Once again, however, it ain't broke, don't fix it.
 */

	for (c = 0; c < scsicnt; c++) {
		if (HBA[c].index < 0) {
			HBA[c].index = bootable - HBA[c].index;
		}
		HBA[HBA[c].index].order = c;
	}
}

static void
strip_quotes(input,output)
char *input, *output;
{
	register char *i, *j;

	for (i=input, j=output; i[0]; i++) {
		if ( i[0] != '"' ) {
			j[0] = i[0];
			j++;
		}
	}
	j[0] = '\0';
}

static int
field_number(name)
char *name;
{
	register int index;

	for (index = 0; index < NUM_FIELDS; index++)
		if (EQUAL(name,field_tbl[index].name))
			return((int)field_tbl[index].tag);

	return(-1);
}

static void
get_values(diskdesc, cfgfp)
struct diskdesc *diskdesc;
FILE *cfgfp;
{
	int		field_number();
	long 	tlong,dcd_ipl_flag,dcd_ivect_flag,dcd_ishare_flag;
	ushort	dcd_ipl,dcd_ishare,dcd_ivect;
	char	ibuffer[BUFSIZ],
			tname[MAX_FIELD_NAME_LEN+1],
			rbuffer[BUFSIZ],
			tbuffer[BUFSIZ];

	dcd_ipl_flag = FALSE;
	dcd_ivect_flag = FALSE;
	dcd_ishare_flag = FALSE;

	do
		if (fgets(ibuffer,BUFSIZ,cfgfp) != NULL) {
			if (sscanf(ibuffer, format, tname, rbuffer) == 2) {
				strip_quotes(rbuffer,tbuffer);
				switch (field_number(tname)) {
				case NAMEL:
					(void)strcpy(diskdesc->fullname,tbuffer);
					if (Debug)
						(void)fprintf(stderr,"NAMEL is %s for %s\n", diskdesc->fullname, diskdesc->name);
					break;
				case DEVICE:
					(void)strcpy(diskdesc->type,tbuffer);
					if (Debug)
						(void)fprintf(stderr,"DEVICE is %s for %s\n", diskdesc->type, diskdesc->name);
					break;
				case DMA2:
					diskdesc->dma2 = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"DMA2 is %hu for %s\n", diskdesc->dma2, diskdesc->name);
					break;
				case IPL:
					if ( diskdesc->ipl == 0 )
						diskdesc->ipl = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IPL is %hu for %s\n", diskdesc->ipl, diskdesc->name);
					break;
				case IVEC:
					if ( diskdesc->ivect == 0 )
						diskdesc->ivect = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IVEC is %hu for %s\n", diskdesc->ivect, diskdesc->name);
					break;
				case SHAR:
					if ( diskdesc->ishare == 0 )
						diskdesc->ishare = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"SHAR is %hu for %s\n", diskdesc->ishare, diskdesc->name);
					break;
				case DCD_IPL:
					dcd_ipl_flag = TRUE;
					dcd_ipl = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IPL is %hu for %s\n", diskdesc->ipl, diskdesc->name);
					break;
				case DCD_IVEC:
					dcd_ivect_flag = TRUE;
					dcd_ivect = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IVEC is %hu for %s\n", diskdesc->ivect, diskdesc->name);
					break;
				case DCD_SHAR:
					dcd_ishare_flag = TRUE;
					dcd_ishare = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"SHAR is %hu for %s\n", diskdesc->ishare, diskdesc->name);
					break;
				}
			}
		}
	while (!feof(cfgfp) && !ferror(cfgfp));

	if (ferror(cfgfp))
		error( "Error occured while reading %s file for module %s.\n",
						CFGNAME, diskdesc->name);

/*
 *	These assignments are done here to ensure that we have read in
 *	the device type before testing it.
 */
	if ( DEVICE_IS_DCD(diskdesc) ) {
		if ( ! dcd_ipl_flag )
			error( "A value for \"DCD_IPL\" is required in the %s file for module %s.\n",CFGNAME, diskdesc->name);
		else if ( ! dcd_ivect_flag )
			error( "A value for \"DCD_IVEC\" is required in the %s file for module %s.\n",CFGNAME, diskdesc->name);
		else if ( ! dcd_ishare_flag )
			error( "A value for \"DCD_SHAR\" is required in the %s file for module %s.\n",CFGNAME, diskdesc->name);

		diskdesc->ipl = dcd_ipl;
		diskdesc->ivect = dcd_ivect;
		diskdesc->ishare = dcd_ishare;
		if ( DEVICE_IS_DISK(diskdesc) ) {
			diskdesc->equip = EQUIP_DISK;
		} else if ( DEVICE_IS_TAPE(diskdesc) ) {
			diskdesc->equip = EQUIP_TAPE;
		}
	} else {
		diskdesc->equip = 0;
	}
}

static int
parse_System_line(diskdesc,ibuffer,direntp)
struct diskdesc *diskdesc;
char *ibuffer;
struct	dirent	*direntp;
{
	int found;
	short dma;
	char	token[BUFSIZ],value[BUFSIZ];

	found = FALSE;

	switch (ibuffer[0]) {

	case '#':
		break;

	case '$':
		if (sscanf(ibuffer,"$%s %s",token,value) == 2) {
			if (EQUAL(token,"version")) {
				diskdesc->version = strtol(ibuffer, (char**)NULL, 0);
			} else if (EQUAL(token,"loadable")) {
				diskdesc->loadable = TRUE;
			}
		}
		break;

	default:
		if (sscanf(ibuffer,"%8s %1s %d %hu %hu %hu %lx %lx %lx %lx %hd",
			diskdesc->name, diskdesc->configure, &diskdesc->unit,
		 	&diskdesc->ipl, &diskdesc->ishare, &diskdesc->ivect,
		 	&diskdesc->sioaddr, &diskdesc->eioaddr,
		 	&diskdesc->smemaddr, &diskdesc->ememaddr, &dma) == 11) {
			if (Debug)
				(void)fprintf(stderr,"%s",ibuffer);
			if (EQUAL(direntp->d_name, diskdesc->name)) {
				found = TRUE;
				diskdesc->dma1 = ((dma < 0)?0:dma);
				diskdesc->configure[0] = 'N';
				(void)strcpy(diskdesc->name,direntp->d_name);
				if(Debug)
					(void)fprintf(stderr,"Found adapter %s, unit %hd.\n",diskdesc->name, diskdesc->unit);
			}
		}
		break;
	}
	return(found);
}

static struct diskdesc *
read_config()
{
	DIR		*parent;
	struct	dirent	*direntp;
	struct diskdesc	*adapters,		/* Chain of adapter descriptions */
					*diskdesc,		/* Current description */
					**nextlink;		/* Next link in adapter chain */
	FILE	*pipefp, *cfgfp;		/* pipe to idinstall */
	unsigned short	found;		/* Found previous sdevice match? */
	short	dma;
	char	command[PATH_MAX+PATH_MAX+10],
			cmd_line[PATH_MAX+PATH_MAX+31],
			tname[MAXNAMELEN+1],
			ibuffer[BUFSIZ],	/* a temp input buffer */
			basename[PATH_MAX+sizeof(PACKD)+1],
			cfgname[PATH_MAX+sizeof(PACKD)+MAXNAMLEN+sizeof(CFGNAME)+3];
	int temp_int;

	if ( access(IDINSTALL, X_OK) ) {
		temp_int = errno;
		if ( Debug )
			fprintf(stderr,"The error number is %d.\n",temp_int);
		errno = 0;
		switch(temp_int) {
			case EPERM:
			case EACCES:
				error("You do not have sufficient privilege to use this command.\n");
			default:
				error("The program %s is not accessible and must be.\n",IDINSTALL);
		}
	}

	if (root == NULL) {
		(void)sprintf(command,"%s -g -s",IDINSTALL);
		(void)sprintf(basename, "%s", PACKD);
	} else {
		(void)sprintf(command,"%s -g -s -R %s/etc/conf",IDINSTALL,root);
		(void)sprintf(basename, "%s%s", root, PACKD);
	}
	(void)sprintf(format, "%%%d[^=]=%%%d[^\n]", MAX_FIELD_NAME_LEN, BUFSIZ-1);

	adapters = NULL;
	nextlink = &adapters;
	adapter_count = 0;

	if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
		error( "Insufficient memory\n");
	diskdesc->next = NULL;
	diskdesc->loadable = FALSE;
	diskdesc->version = 0;

	if ((parent = opendir(basename)) == NULL)
			error( "Could not process directory %s\n", basename);

/*
 *	This while loop can be read as "for each entry in the pack.d directory"
 */
	while ((direntp = readdir( parent )) != NULL) {

		(void)sprintf(cfgname, "%s/%s/%s", basename, direntp->d_name, CFGNAME);

		if (access(cfgname, F_OK))		/* if there is no disk.cfg file here */
			continue;

		if ( Debug )
			sprintf(cmd_line,"%s %s",command,direntp->d_name);
		else
			sprintf(cmd_line,"%s %s 2>/dev/null",command,direntp->d_name);

		if ((pipefp = popen(cmd_line, "r")) == NULL) 
			error( "Cannot read System entry for module %s.\n", direntp->d_name);

		if ((cfgfp = fopen(cfgname, "r")) == NULL) 
			error( "Cannot open file %s for reading.\n", cfgname);

		found = FALSE;

		do {
			if (fgets(ibuffer,BUFSIZ,pipefp) != NULL) {
				if ( parse_System_line(diskdesc, ibuffer, direntp) ) {
					found++;
					adapter_count++;

					get_values(diskdesc, cfgfp);
					rewind(cfgfp);

					*nextlink = diskdesc;			/* Add to end of chain */
					nextlink = &diskdesc->next;		/* Point to next link */

					if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
						error( "Insufficient memory\n");
					diskdesc->next = NULL;
					diskdesc->loadable = FALSE;
					diskdesc->version = 0;
				}
			}
		} while (!feof(pipefp) && !ferror(pipefp));

		if (!found)
			error( "No System entry for module %s.\n", direntp->d_name);

		if (pclose(pipefp) == -1)
			error( "Problem closing pipe to %s\n", cmd_line);

		if (fclose(cfgfp) == -1)
			error( "Problem closing file %s\n", cfgname);

	}

	(void)closedir( parent );

	return (adapters);
}

static char *
get_name(inquiry_string, unit)
char	*inquiry_string;
int	*unit;
{
	register char *paren;

	if (inquiry_string[0] != '(') {
		*unit = -1;
		return NULL;
	}

	if ((paren = strchr(inquiry_string, ')')) == NULL)
		error("No device name found in %s.\n",inquiry_string);

	paren[0] = '\0';

	if ((paren = strchr(inquiry_string, ',')) == NULL)
		*unit = 1;
	else {
		*unit = (int)strtol(&paren[1], (char**)NULL, 0);
		paren[0] = '\0';
	}

	if ((paren = strchr(inquiry_string, '(')) == NULL)
		error("No device name found in %s.\n",inquiry_string);

	return(++paren);
}

static void
force_configure(adapters)
struct diskdesc *adapters;
{
	struct diskdesc *device;

	if (Debug)
		(void)fprintf(stderr,"Marking all adapters as configure = Y.\n");

	for (device = adapters; device != NULL; device = device->next) {
		if (Debug)
			(void)fprintf(stderr,"\tMarking %s as configured in.\n",device->name);
		device->configure[0] = 'Y';
		device->unit = -1;
	}
}

static void
force_individual(adapters)
struct diskdesc *adapters;
{
	struct diskdesc *device;
	int index;

	for (index=0; index < force_flag; index++) {
		for (device = adapters; device != NULL; device = device->next) {
			if (EQUAL(device->name,force[index])) {
				if (Debug)
					(void)fprintf(stderr,"Forcing %s to be configured in.\n",device->name);
				device->configure[0] = 'Y';
				device->unit = -1;
				break;
			}
		}
		if (device == NULL) {
			errno = 0;
			warning("A device driver for %s was not found on this system.\n", force[index]);
		}
	}
}

static void
mark_configure(adapters, tc_name, tc_unit, equipment, controller)
struct diskdesc *adapters;
char *tc_name;
int tc_unit, controller;
unsigned long equipment;
{
	struct diskdesc *device;
	int	unit;

	if (Debug)
		(void)fprintf(stderr,"Name found is %s, unit %hd\n", (tc_name?tc_name:"NULL"), tc_unit);

	if ( tc_name == NULL ) {
		return;
	}

	unit = tc_unit;

	for (device = adapters; device != NULL; device = device->next) {
		if (EQUAL(device->name,tc_name)) {
			if ( --unit == 0 ) {
				if (Debug)
					(void)fprintf(stderr,"Marking %s number %d as configured in.\n",tc_name,tc_unit);
				device->configure[0] = 'Y';
				device->unit = controller;
				device->equip = equipment;
				device->active++;
				return;
			}
		}
	}
	error("No entry for %s number %d found in current configuration.\n",tc_name,tc_unit);
}

static void
write_output(adapters)
struct diskdesc *adapters;
{
	struct diskdesc *diskdesc;
	if (Debug) {
		(void)fprintf(stderr,"name\t\"fullname\"\t\ttype\tconf\tunit\tequip\tdma1\tdma2\tipl\tivect\tishare\tsio\teio\tsmem\temem\n");
	}

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {
		if (Debug) {
			(void)fprintf(stderr,"%s\t\"%s\"\t%s\t%s\t%d\t0x%lx\t%hu\t%hu\t%hu\t%hu\t%hu\t0x%lx\t0x%lx\t0x%lx\t0x%lx\n",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       diskdesc->unit, diskdesc->equip,
			       diskdesc->dma1, diskdesc->dma2,
			       diskdesc->ipl, diskdesc->ivect,
			       diskdesc->ishare,
			       diskdesc->sioaddr, diskdesc->eioaddr,
			       diskdesc->smemaddr, diskdesc->ememaddr);
		}
		(void)printf("%s\t\"%s\"\t%s\t%s\t%d\t0x%lx\t%hu\t%hu\t%hu\t%hu\t%hu\t0x%lx\t0x%lx\t0x%lx\t0x%lx\n",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       diskdesc->unit, diskdesc->equip,
			       diskdesc->dma1, diskdesc->dma2,
			       diskdesc->ipl, diskdesc->ivect,
			       diskdesc->ishare,
			       diskdesc->sioaddr, diskdesc->eioaddr,
			       diskdesc->smemaddr, diskdesc->ememaddr);
	}
}

static int
alpha_compare(device1,device2)
struct diskdesc *device1, *device2;
{
/*
 * SCSI devices sort higher than DCD devices
 */
	if (DEVICE_IS_DCD(device1) && DEVICE_IS_DCD(device2))
		return(FALSE);

	if (DEVICE_IS_SCSI(device1) && DEVICE_IS_DCD(device2))
		return(FALSE);

	if (DEVICE_IS_DCD(device1) && DEVICE_IS_SCSI(device2))
		return(TRUE);
/*
 * now we know that they are both SCSI devices,
 * lets compare their names
 */
	return((strcmp(device1->name,device2->name)<=0)?FALSE:TRUE);
}

static int
EDT_compare(device1,device2)
struct diskdesc *device1, *device2;
{
/*
 * DCD devices sort the same
 */
	if (DEVICE_IS_DCD(device1) && DEVICE_IS_DCD(device2))
		return(FALSE);

/*
 * now we know that they are both NOT dcd devices,
 * lets compare the active flags.
 */
	if (DEVICE_IS_NOT_ACTIVE(device1) && DEVICE_IS_NOT_ACTIVE(device2))
		return(FALSE);

	if (DEVICE_IS_ACTIVE(device1) && DEVICE_IS_NOT_ACTIVE(device2))
		return(FALSE);

	if (DEVICE_IS_NOT_ACTIVE(device1) && DEVICE_IS_ACTIVE(device2))
		return(TRUE);
/*
 * both devices are active, lets compare equipment
 */
	if (DEVICE_HAS_DISK(device1) && !DEVICE_HAS_DISK(device2))
		return(FALSE);

	if (!DEVICE_HAS_DISK(device1) && DEVICE_HAS_DISK(device2))
		return(TRUE);
/*
 * now, either both devices have disk or neither device
 * has disk, final test is EDT order.  The lower
 * unit number is first in the EDT.  Negative units
 * are not in the EDT yet.
 */
	if (device1->unit < 0 && device2->unit < 0)
		return(FALSE);

	if (device1->unit < 0 )
		return(TRUE);

	if (device2->unit < 0 )
		return(FALSE);

	if (device1->unit < device2->unit)
		return(FALSE);

	return(TRUE);
}

static struct diskdesc *
adapter_sort(adapters,edtptr,compare)
struct diskdesc *adapters;
EDT				*edtptr;
int 			(*compare)();
{
	struct diskdesc	*adapter, *top, *prev, *temp;
	int				j, i, sorted, lun, order, save_pdtypes;
	register EDT	*edtptr2;
	char			*tc_name;
	int	tc_unit;

	if ( adapter_count == 1 )	/* if there is only one, no need to sort */
		return(adapters);

/*
 * here we malloc another diskdesc structure so we can have
 * a dummy one to use to point to the beginning of the adapters.
 * This simplifies the sort algorithm immensely.
 */
	if ((top = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
		error( "Insufficient memory\n");
	top->next = adapters;
/*
 * This is simply a dumb bubble-sort.  Since the adapters structure
 * is relatively small, this will suffice.  At the present time,
 * small means 8 or 9 devices.
 */
	sorted = FALSE;

	while ( !sorted ) {
		sorted = TRUE;
		adapter = top->next;
		prev = top;
		while ( adapter->next != NULL ) {
			if ( (*compare)(adapter,adapter->next) ) {  /* do we need to bubble */
				sorted = FALSE;
				temp = adapter->next->next;
				adapter->next->next = prev->next;
				prev->next = adapter->next;
				adapter->next = temp;
				prev = prev->next;
			} else {			/* go to the next one */
				prev = adapter;
				adapter = adapter->next;
			}
		}
	}

	return(top->next);
}

main(argc,argv)
int	argc;
char	**argv;
{
	register EDT 	*xedtptr  = NULL;	 /* Pointer to edt */
	register EDT 	*xedtptr2 = NULL;	 /* Temp pointer   */
	register int	c, t;
	char			*tc_name;
	int	tc_unit;
	unsigned long	equipment;
	int				arg;
	struct diskdesc	*adapters;		/* Chain of adapter descriptions */
	
	Cmdname = strdup(argv[0]);
	
	Debug = FALSE;
	root = NULL;
	err_flag = FALSE;
	force_flag = FALSE;
	all_flag = FALSE;

	opterr = 0;		/* turn off the getopt error messages */

	while ((arg = getopt(argc,argv,"aSR:f:")) != EOF) {

		switch (arg) {
		case 'a' : /* configure in all pdi devices */
			all_flag = TRUE;
			break;
		case 'f' : /* configure in the specified pdi device */
			if (force_flag >= MAX_FORCE)
				error("%s: Too many -f options specified, try again with no more than %d.\n", Cmdname, MAX_FORCE);
			force[force_flag++] = strdup(optarg);
			break;
		case 'S' : /* Turn on debug messages */
			Debug = TRUE;
			break;
		case 'R' : /* set the root variable */
			root = strdup(optarg);
			break;
		case '?' : /* Incorrect argument found */
			error("usage: %s [-aS] [-f device] [-R root] [outputfile]\n",Cmdname);
			break;
		}
	}

/*
 *	If there is an argument left, use it to name the output file
 *	instead of using stdout.
 */
	if (optind < argc)
		(void)freopen(argv[optind], "w", stdout);

	/* Ignore certain signals */
	if (!Debug) {
		(void) signal(SIGHUP,SIG_IGN);
		(void) signal(SIGINT,SIG_IGN);
		(void) signal(SIGTERM,SIG_IGN);
	}
	umask(0); /* use template file permission (mode) */

/*
 *	Read in all of the configuration info on all PDI devices in this system.
 *  This consists of all devices that have a disk.cfg file in their pack.d
 *
 *	Besides reading in the disk.cfg file, read in the sdevice.d entry as well.
 *
 *	If there is more than one entry in the sdevice.d file for any
 *	device, there is more than one entry in adapters as well.
 */
	adapters = read_config();
	if (adapters == NULL)
		error("%s: No PDI devices found on system.\n", Cmdname);

	if ( all_flag ) {
		force_configure(adapters);

		/*
		 * Now we sort the adapters into alphabetic order, making
		 * SCSI devices first.
		 */
		adapters = adapter_sort(adapters,NULL,alpha_compare);

	} else {
		if ((xedtptr = readedt(&scsicnt)) == 0) {
			error("%s: Unable to read system Equipped Device Table.\n", Cmdname);
		}

		reorder_config(xedtptr,scsicnt);  /* set-up the HBA array */

		if (Debug) {
			(void)fprintf(stderr,"\ndriver\tHA\tTC\tnumlus\tPDtype\tequip\tTCinq\n");
		}
	
		for (c = 0; c < scsicnt; c++) {

			xedtptr2 = HBA[HBA[c].order].edtptr;

			for (equipment = 0, t = 0; t < MAX_TCS; t++, xedtptr2++) {

				if (Debug) {
					(void)fprintf(stderr,"%s\t%d\t%d\t%d\t%d\t%d\t%s\n",
						xedtptr2->drv_name, c, t,
						xedtptr2->n_lus, xedtptr2->pdtype,
						xedtptr2->tc_equip, xedtptr2->tc_inquiry);
				}

				/*
				 * record the device types attached to this HBA
				 * we use this info below to sort the entries
				 */
				if ( xedtptr2->tc_equip ) {
					equipment |= (1 << (xedtptr2->pdtype));
					tc_name = get_name((char *) xedtptr2->tc_inquiry, &tc_unit);
					mark_configure(adapters, tc_name, tc_unit, equipment, c);
				}
			}
		}

		/*
		 * Now turn on any adapters specified on the command
		 * line with the -f option.
		 */
		if ( force_flag ) {
			force_individual(adapters);
		}
/*
 * Instead of getting the HBA_tbl to get the order of the installed devices,
 * we only need to use the order already supplied by the EDT!  This is
 * already in the unit field of the diskdesc structure.
 *
 * Now we sort the adapters into EDT order, making
 * disk devices first.
*/
		adapters = adapter_sort(adapters,xedtptr,EDT_compare);
	}

	write_output(adapters);

	(void)fclose(stdout);

	exit(NORMEXIT);
}
