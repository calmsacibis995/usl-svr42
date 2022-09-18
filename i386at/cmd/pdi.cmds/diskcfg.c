/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:diskcfg.c	1.9"
#ident	"$Header: miked 5/26/92$"

/*
 * Disk driver configuration utility
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/vtoc.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/scsicomm.h>
#include "diskcfg.h"

#define DCFG_MODE	0644
#define DCFG_OWN	0	/* root */
#define DCFG_GRP	3	/* sys  */

static char	*progname,
			*root,
			*dcdbase;
static char temp_dir[L_tmpnam];
static char iformat[50];

extern int	opterr,
			optind;
extern char	*optarg;

extern int Debug = FALSE;

extern void		error();
static struct diskdesc	*read_input();
static void		read_Config();
static void		strip_quotes();
static void		validate_Config();
static void		set_defaults();
static void		get_values();
static int		make_System_loadable();
static void		build_System_files();
static void		update_configuration();
static void		update_Config();

main(argc, argv)
int	argc;
char	*argv[];
{
	struct diskdesc	*adapters;		/* Chain of adapter descriptions */
	int	arg, loadable_dcd;

	opterr = 0;	/* turn off getopt error messages */

	progname = argv[0];

	root = NULL;

	while ((arg = getopt(argc,argv,"SR:")) != EOF) {

		switch (arg) {
		case 'S' : /* Turn on Debug messages */
			Debug = TRUE;
			break;
		case 'R' : /* set the root variable */
			root = strdup(optarg);
			break;
		case '?' : /* Incorrect argument found */
			error("usage: %s [-S] [-R root] [inputfile]\n",progname);
			break;
		}
	}

/*
 *	If there is an arguement left, make it the input file
 *	instead of stdin.
 */
	if (optind < argc)
		(void)freopen(argv[optind], "r", stdin);

	if ((adapters = read_input()) == NULL)
		return (NORMEXIT);

	read_Config(adapters);

	validate_Config(adapters);

	loadable_dcd = make_System_loadable(adapters);

	build_System_files(adapters,loadable_dcd);

	update_configuration(adapters);

	update_Config(adapters);

	return (NORMEXIT);
}

static struct diskdesc *
read_input()
{
	char		*ptr;			/* String pointer */
	unsigned short	lineno;			/* Input line number */
	int		retval;			/* Return value from scanf */
	struct diskdesc	*adapters,		/* Chain of adapter descriptions */
			*diskdesc,		/* Current description */
			*tmpdesc;		/* Description for searching */
	struct diskdesc	**nextlink;		/* Next link in adapter chain */

	adapters = NULL;
	nextlink = &adapters;
	lineno = 1;

	for (;;) {
		if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
			error( "Insufficient memory\n");

		diskdesc->next = NULL;

		retval = scanf("%8s \"%64[^\"]\" %4s %1s %d %lx %hu %hu %hu %hu %hu %lx %lx %lx %lx",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       &diskdesc->unit, &diskdesc->equip,
			       &diskdesc->dma1, &diskdesc->dma2,
			       &diskdesc->ipl, &diskdesc->ivect,
			       &diskdesc->ishare,
			       &diskdesc->sioaddr, &diskdesc->eioaddr,
			       &diskdesc->smemaddr, &diskdesc->ememaddr);
		if (retval == EOF)
			return (adapters);

		if (retval != ARGSPERLINE)
			error( "Invalid field %d in line %u of input\n",
				   retval+1, lineno);

		for (ptr = diskdesc->type; *ptr != '\0'; ptr++)
			if (isupper(*ptr))
				*ptr = toupper(*ptr);
		if (!DEVICE_IS_SCSI(diskdesc) && !DEVICE_IS_DCD(diskdesc))
			error( "Unrecognized adapter type %s in line %u of input\n",
				   diskdesc->type, lineno);

		if (strchr(VALIDCONF, diskdesc->configure[0]) == NULL)
			error( "Unrecognized configuration flag %c in line %u of input\n",
				   diskdesc->configure[0], lineno);
		if (islower(diskdesc->configure[0]))
			diskdesc->configure[0] = toupper(diskdesc->configure[0]);

		if (diskdesc->ishare < MINISHARE || diskdesc->ishare > MAXISHARE)
			error( "Unrecognized interrupt sharing code %u in line %u of input\n",
				   diskdesc->ishare, lineno);

		if (diskdesc->sioaddr > diskdesc->eioaddr)
			error( "Starting I/O address %x exceeds ending I/O address %x in line %u of input\n",
				   diskdesc->sioaddr, diskdesc->eioaddr, lineno);

		if (diskdesc->smemaddr > diskdesc->ememaddr)
			error( "Starting memory address %x exceeds ending memory address %x in line %u of input\n",
				   diskdesc->smemaddr, diskdesc->ememaddr, lineno);

		*nextlink = diskdesc;			/* Add to end of chain */
		nextlink = &diskdesc->next;		/* Point to next link */
		lineno++;
	}
}

/*
 * Extract configuration information from the disk.cfg file for each adapter.
 *
 * we only bother to do this for the devices we are configuring in.
 */
static void
read_Config(adapters)
struct diskdesc	*adapters;
{
	FILE	*cfgfp;		/* fp to disk.cfg file */
	struct diskdesc	*diskdesc;	/* Description for searching */
	char	basename[PATH_MAX+sizeof(PACKD)+1],
			cfgname[PATH_MAX+sizeof(PACKD)+MAXNAMELEN+sizeof(CFGNAME)+3],
			tname[MAXNAMELEN+1],
			ibuffer[BUFSIZ];	/* a temp input buffer */


	if (root == NULL)
		(void) sprintf(basename, "%s", PACKD);
	else
		(void) sprintf(basename, "%s%s", root, PACKD);

	(void)sprintf(iformat, "%%%d[^=]=%%%d[^\n]", MAX_FIELD_NAME_LEN, BUFSIZ-1);

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {

		if (!CONFIGURE(diskdesc))
			continue;

		sprintf(cfgname,"%s/%s/%s",basename,diskdesc->name,CFGNAME);
		if ((cfgfp = fopen(cfgname, "r")) == NULL) 
			error( "Cannot open file %s for reading.\n", cfgname);

		set_defaults(diskdesc);

		get_values(diskdesc, cfgfp);

		if (fclose(cfgfp) == -1)
			error( "Problem closing file %s\n", cfgname);

	}
}

static void
set_defaults(diskdesc)
struct diskdesc *diskdesc;
{
	diskdesc->devtype = strdup(D_DEVTYPE);
}

static void
get_values(diskdesc, cfgfp)
struct diskdesc *diskdesc;
FILE *cfgfp;
{
	int		field_number();
	long 	tlong;
	char	ibuffer[BUFSIZ],
			tname[MAX_FIELD_NAME_LEN+1],
			rbuffer[BUFSIZ],
			tbuffer[BUFSIZ];

	do
		if (fgets(ibuffer,BUFSIZ,cfgfp) != NULL) {
			if (sscanf(ibuffer, iformat, tname, rbuffer) == 2) {
				strip_quotes(rbuffer,tbuffer);
				switch (field_number(tname)) {
					case DEVTYPE:
						diskdesc->devtype = strdup(tbuffer);
						if (Debug)
							(void)fprintf(stderr,"DEVTYPE is %s for %s\n", diskdesc->devtype, diskdesc->name);
						break;
				}
			}
		}
	while (!feof(cfgfp) && !ferror(cfgfp));

	if (ferror(cfgfp))
		error( "Error occured while reading %s file for module %s.\n",
						CFGNAME, diskdesc->name);

}

static void
set_new_value(ibuffer,field_number,value)
char *ibuffer;
int field_number;
unsigned short value;
{
	(void)sprintf(ibuffer,"%s=%d\n",field_tbl[--field_number].name,value);
}

static void
set_Config_values(diskdesc, cfgfp)
struct diskdesc *diskdesc;
{
	FILE	*ncfgfp,*ocfgfp;
	int		field_number();
	int		ipl_done,ivect_done,ishare_done;
	long 	tlong;
	char	basename[PATH_MAX+sizeof(PACKD)+1],
			ncfgname[PATH_MAX+sizeof(PACKD)+MAXNAMELEN+sizeof(CFGNAME)+3],
			ocfgname[PATH_MAX+sizeof(PACKD)+MAXNAMELEN+sizeof(CFGNAME)+3],
			ibuffer[BUFSIZ],
			tname[MAX_FIELD_NAME_LEN+1],
			rbuffer[BUFSIZ],
			tbuffer[BUFSIZ];
	char	command[PATH_MAX+PATH_MAX+26],
			cmd_line[PATH_MAX+PATH_MAX+PATH_MAX+42];

	if (root == NULL)
		(void) sprintf(basename, "%s", PACKD);
	else
		(void) sprintf(basename, "%s%s", root, PACKD);

	sprintf(ocfgname,"%s/%s/%s",basename,diskdesc->name,CFGNAME);
	sprintf(ncfgname,"%s/%s/T%s",basename,diskdesc->name,CFGNAME);
	sprintf(cmd_line,"%s %s %s",MV,ocfgname,ncfgname);
	if ((tlong = system(cmd_line)) == -1) 
		error( "Error %d trying to %s %s file.\n", errno, MV, CFGNAME);

	if ((ncfgfp = fopen(ncfgname, "r")) == NULL) 
		error( "Cannot open file %s for input.\n", ncfgname);

	if ((ocfgfp = fopen(ocfgname, "w")) == NULL) 
		error( "Cannot open file %s for output.\n", ocfgname);

	ipl_done=0;
	ivect_done=0;
	ishare_done=0;

	do {
		if (fgets(ibuffer,BUFSIZ,ncfgfp) != NULL) {
			if (sscanf(ibuffer, iformat, tname, rbuffer) == 2) {
				switch (field_number(tname)) {
					case DCD_IPL:
						++ipl_done;
						set_new_value(ibuffer,DCD_IPL,diskdesc->ipl);
						if (Debug)
							(void)fprintf(stderr,"Changing DCD_IPL to %d for %s\n", diskdesc->ipl, diskdesc->name);
						break;
					case DCD_IVEC:
						++ivect_done;
						set_new_value(ibuffer,DCD_IVEC,diskdesc->ivect);
						if (Debug)
							(void)fprintf(stderr,"Changing DCD_IVEC to %d for %s\n", diskdesc->ivect, diskdesc->name);
						break;
					case DCD_SHAR:
						++ishare_done;
						set_new_value(ibuffer,DCD_SHAR,diskdesc->ishare);
						if (Debug)
							(void)fprintf(stderr,"Changing DCD_SHAR to %d for %s\n", diskdesc->ishare, diskdesc->name);
						break;
				}
			}
			(void)fputs(ibuffer,ocfgfp);
		}
	 } while (!feof(ncfgfp) && !ferror(ncfgfp) && !ferror(ocfgfp));

	if (ferror(ncfgfp))
		error( "Error occured while reading T%s file for module %s.\n",
						CFGNAME, diskdesc->name);

	if (!ipl_done) {
		set_new_value(ibuffer,DCD_IPL,diskdesc->ipl);
		(void)fputs(ibuffer,ocfgfp);
	}
	if (!ishare_done) {
		set_new_value(ibuffer,DCD_SHAR,diskdesc->ishare);
		(void)fputs(ibuffer,ocfgfp);
	}
	if (!ivect_done) {
		set_new_value(ibuffer,DCD_IVEC,diskdesc->ivect);
		(void)fputs(ibuffer,ocfgfp);
	}

	if (ferror(ocfgfp))
		error( "Error occured while writing %s file for module %s.\n",
						CFGNAME, diskdesc->name);

	if (fclose(ncfgfp) == -1)
		error( "Problem closing file %s\n", ncfgname);
	if (fclose(ocfgfp) == -1)
		error( "Problem closing file %s\n", ocfgname);

	if (ferror(ocfgfp))
		error( "Error occured while writting %s file for module %s.\n",
						CFGNAME, diskdesc->name);

	(void)unlink(ncfgname);

	(void)chmod(ocfgname, DCFG_MODE);
	(void)chown(ocfgname, (uid_t)DCFG_OWN, (gid_t)DCFG_GRP);
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

/*
 * validate configuration information obtained from the disk.cfg file for each adapter.
 *
 * we only bother to do this for the devices we are configuring in.
 */
static void
validate_Config(adapters)
struct diskdesc	*adapters;
{
	struct diskdesc	*device1;	/* Description for searching */

	for (device1 = adapters; device1 != NULL; device1 = device1->next) {

		if (CONFIGURE(device1) && DEVICE_IS_DCD(device1) && EQUAL(device1->devtype,D_DEVTYPE))
			error( "No device type value found in %s file for module %s.\n",
							CFGNAME, device1->name);
	}
}

static void
write_Non_SCSI_System_line(diskdesc,sdevfp,sdevname)
struct diskdesc	*diskdesc;
FILE *sdevfp;
char *sdevname;
{
	if (fprintf(sdevfp, "%s\t%c\t%d\t%u\t%u\t%u\t%lx\t%lx\t%lx\t%lx\t%hd\n",
	    	diskdesc->name, diskdesc->configure[0], diskdesc->unit,
			0, 0, 0,
	    	diskdesc->sioaddr, diskdesc->eioaddr,
	    	diskdesc->smemaddr, diskdesc->ememaddr,
			(diskdesc->dma1 ? diskdesc->dma1 : -1) ) < 0)
		error( "Problem writing to file %s.\n", sdevname);
}

static void
write_SCSI_System_line(diskdesc,sdevfp,sdevname)
struct diskdesc	*diskdesc;
FILE *sdevfp;
char *sdevname;
{
	if (fprintf(sdevfp, "%s\t%c\t%d\t%u\t%u\t%u\t%lx\t%lx\t%lx\t%lx\t%hd\n",
		    diskdesc->name, diskdesc->configure[0], diskdesc->unit,
			SDI_IPL, diskdesc->ishare, diskdesc->ivect,
	    	diskdesc->sioaddr, diskdesc->eioaddr,
	    	diskdesc->smemaddr, diskdesc->ememaddr,
			diskdesc->dma1 ? diskdesc->dma1 : -1 ) < 0)
		error( "Problem writing to file %s.\n", sdevname);
}

/*
 * Build all the System files.  We build them for the following:
 *	dcd
 *	each adapter
 */
static void
build_System_files(adapters,loadable_dcd)
struct diskdesc	*adapters;
int loadable_dcd;
{
	char	sdevname[L_tmpnam+MAXNAMELEN+2+sizeof(SYSTEM)],
			dcdname[L_tmpnam+sizeof(DCDNAME)+2+sizeof(SYSTEM)];
	int		temp, dcd_ints, dcd_unit, written_static_yet, first_time;
	struct diskdesc *diskdesc;
	FILE		*dcdfp,
				*sdevfp;

	if (mkdir(tmpnam(temp_dir), S_IRWXU) != 0)
		error( "Cannot create temporary directory %s.\n",
						temp_dir);

	(void) sprintf(dcdname, "%s/%s", temp_dir, DCDNAME);
	if (mkdir(dcdname, S_IRWXU) != 0)
		error( "Cannot create temporary directory %s.\n",dcdname);
	dcdbase = strdup(dcdname);
	(void) sprintf(dcdname, "%s/%s/%s", temp_dir, DCDNAME, SYSTEM);
	if ((dcdfp = fopen(dcdname, "w")) == NULL) 
		error( "Cannot open file %s for write.\n", dcdname);

	if (fprintf(dcdfp, "$version 1\n") < 0)
		error( "Problem writing to file %s.\n", dcdname);

	if (loadable_dcd) {
		if (fprintf(dcdfp,"$loadable %s\n",DCDNAME) < 0)
			error( "Problem writing to file %s.\n", dcdname);
	}

	for (dcd_unit=-1, dcd_ints=0, diskdesc=adapters;
		 diskdesc != NULL;
		 diskdesc = diskdesc->next) {

		(void) sprintf(sdevname, "%s/%s", temp_dir, diskdesc->name);

		if (access(sdevname,F_OK) != 0) {
			first_time = TRUE;
			diskdesc->Install_path = strdup(sdevname);
			if (mkdir(sdevname, S_IRWXU) != 0)
				error( "Cannot create temporary directory %s.\n",
								sdevname);
		} else {
			first_time = FALSE;
			diskdesc->Install_path = NULL;
		}

		(void) sprintf(sdevname, "%s/%s/%s", temp_dir, diskdesc->name, SYSTEM);
		if ((sdevfp = fopen(sdevname, "a")) == NULL) 
			error( "Cannot open file %s for append.\n", sdevname);

		if ( first_time ) {
			if (fprintf(sdevfp, "$version 1\n") < 0)
				error( "Problem writing to file %s.\n", sdevname);
		}

		if (diskdesc->loadable == MAYBE) {
			diskdesc->loadable = loadable_dcd;
		}

		if (first_time && diskdesc->loadable) {
			if (fprintf(sdevfp,"$loadable %s\n",diskdesc->name) < 0)
				error( "Problem writing to file %s.\n", sdevname);
		}

		if (DEVICE_IS_SCSI(diskdesc)) {
			write_SCSI_System_line(diskdesc,sdevfp,sdevname);
		} else if (DEVICE_IS_DCD(diskdesc)) {
			write_Non_SCSI_System_line(diskdesc,sdevfp,sdevname);
			temp = (1 << diskdesc->ivect);
			if (CONFIGURE(diskdesc) && !(temp & dcd_ints)) {
				dcd_ints |= temp;
				dcd_unit++;
				if (fprintf(dcdfp,
						"%s\tY\t%d\t%u\t%u\t%u\t%lx\t%lx\t%lx\t%lx\t%hd\n",
			   			DCDNAME, diskdesc->unit,
						SDI_IPL, diskdesc->ishare, diskdesc->ivect,
			   			0, 0, 0, 0, -1 ) < 0) {
					error( "Problem writing to file %s.\n", dcdname);
				}
			}
		}

		if (fclose(sdevfp) != 0)
			error( "Problem closing file %s.\n", sdevname);
	}

	/* If no direct coupled devices, make sure we turn off the dcd module */
	if ( dcd_unit == -1 ) {
		if (fprintf(dcdfp, "dcd\tN\t1\t0\t0\t0\t0\t0\t0\t0\t-1\n") < 0)
				error( "Problem writing to file %s\n", dcdname);
	} 

	if (fclose(dcdfp) != 0)
		error( "Problem closing file %s\n", dcdname);

}

/*
 * Make all the decisions about loadable System files.
 *
 * We do this for the following:
 *	dcd
 *	each adapter(dcd-device or scsi-hba)
 */
static int
make_System_loadable(adapters)
struct diskdesc	*adapters;
{
	int		written_static_yet, loadable_dcd;
	struct diskdesc	*diskdesc;

	for (loadable_dcd=TRUE, written_static_yet=FALSE, diskdesc=adapters;
		 diskdesc != NULL;
		 diskdesc = diskdesc->next) {

		if (CONFIGURE(diskdesc) ) {
			if (DEVICE_IS_SCSI(diskdesc)) {
				if (DEVICE_HAS_DISK(diskdesc)) {
					diskdesc->loadable = written_static_yet;
					if (!written_static_yet) {
						written_static_yet = TRUE;
					}
				} else {
					diskdesc->loadable = TRUE;
				}
			} else if (DEVICE_IS_DCD(diskdesc)) {
				if (written_static_yet) {
					diskdesc->loadable = loadable_dcd;
				} else {
					if (DEVICE_IS_DISK(diskdesc)) {
						written_static_yet = TRUE;
						loadable_dcd = FALSE;
						diskdesc->loadable = FALSE;
					} else {
						diskdesc->loadable = MAYBE;
					}
				}
			}
		} else {
			diskdesc->loadable = TRUE;  /* ul92-09927 */
		}
	}
/*
 *	The MAYBE above deserves a bit of explanation.  In fact, at this
 *	point, the correct value for any and all devices that now
 *	say MAYBE is loadable_dcd but I just don't want to loop thru
 *	the whole thing again here since I have to loop through it all
 *	again when I write them all out.
 */

	return(loadable_dcd);

}

/*
 * update configuration for each adapter and dcd
 */
static void
update_configuration(adapters)
struct diskdesc	*adapters;
{
	int	idretval;
	struct diskdesc	*diskdesc;
	char	command[PATH_MAX+PATH_MAX+26],
			cmd_line[PATH_MAX+PATH_MAX+PATH_MAX+42];

	if (root == NULL) {
		sprintf(command,"%s -u -e",IDINSTALL);
	} else {
		sprintf(command,"%s -u -e -R %s/etc/conf",IDINSTALL,root);
	}

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {

		if ( diskdesc->Install_path == NULL )
			continue;

		if ( Debug ) {
			sprintf(cmd_line,"cd %s;%s %s",
				diskdesc->Install_path,command,diskdesc->name);
		} else {
			sprintf(cmd_line,"cd %s;%s %s 2>/dev/null",
				diskdesc->Install_path,command,diskdesc->name);
		}

		idretval = system(cmd_line);

		if ( idretval == -1 )
			error( "Cannot fork shell to execute %s.\n",
							cmd_line);

		if ( WEXITSTATUS(idretval) != 0 )
			error( "Unable to update configuration for %s.\n", diskdesc->name);

		(void) rmdir(diskdesc->Install_path);
	}

	if ( Debug ) {
		sprintf(cmd_line,"cd %s;%s %s",
			dcdbase,command,DCDNAME);
	} else {
		sprintf(cmd_line,"cd %s;%s %s 2>/dev/null",
			dcdbase,command,DCDNAME);
	}
	idretval = system(cmd_line);
	if ( idretval == -1 )
		error( "Cannot fork shell to execute %s.\n", cmd_line);
	if ( WEXITSTATUS(idretval) != 0 )
		error( "Unable to update configuration for %s.\n", DCDNAME);
	(void) rmdir(dcdbase);

	(void) rmdir(temp_dir);
}

/*
 * Save the ivec, shar and ipl for the DCD dev's in their dosk.cfg files.
 *
 * we only bother to do this for the devices we are configuring in.
 */
static void
update_Config(adapters)
struct diskdesc	*adapters;
{
	FILE	*cfgfp;		/* fp to disk.cfg file */
	struct diskdesc	*diskdesc;	/* Description for searching */
	char	basename[PATH_MAX+sizeof(PACKD)+1],
			cfgname[PATH_MAX+sizeof(PACKD)+MAXNAMELEN+sizeof(CFGNAME)+3],
			tname[MAXNAMELEN+1],
			ibuffer[BUFSIZ];	/* a temp input buffer */


	if (root == NULL)
		(void) sprintf(basename, "%s", PACKD);
	else
		(void) sprintf(basename, "%s%s", root, PACKD);


	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {

		if (!CONFIGURE(diskdesc) || DEVICE_IS_SCSI(diskdesc))
			continue;

/*
 *
 *		set_Config_values(diskdesc, cfgfp);
 *
 *		read thru disk.cfg
 *			variable of interest ?
 *				replace the line buffer with the new value
 *			write out the line
 */

 		set_Config_values(diskdesc, cfgfp);
	}
}
