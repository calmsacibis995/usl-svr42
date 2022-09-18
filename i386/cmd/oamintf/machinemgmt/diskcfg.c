/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:i386/cmd/oamintf/machinemgmt/diskcfg.c	1.1"
#ident	"$Header: $"

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
#include <userdefs.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#define	IDINSTALL	"/etc/conf/bin/idinstall -u -m"
#define	MASTER		"Master"

#define	MAXNAMELEN	8

#define	MAXFULLNAMELEN	64

#define	MAXTYPELEN	4
#define	SCSITYPE	"SCSI"
#define	DCDTYPE		"DCD"

#define	VALIDCONF	"YyNn"

#define	NOISHARE	1
#define	ISHARESAME	2
#define	ISHAREDIFF	3
#define	MINISHARE	1
#define	MAXISHARE	1

#define	ARGSPERLINE	13

#define	MAXPREFIXLEN	6



#define	SDIREPL		"_HBA_TBL" 
#define SDIREPLLEN	(sizeof(SDIREPL) - 1)

#define	DCDREPL		"_DCD_TBL"
#define DCDREPLLEN	(sizeof(DCDREPL) - 1)


struct diskdesc {
	struct diskdesc	*next;			/* Next in chain */
	struct mdevline	*mdevline;		/* mdevice entry */
	/* These are read from user input */
	char		name[MAXNAMELEN+1];	/* Name of driver */
	char		fullname[MAXFULLNAMELEN+1];	/* Human-readable name */
	char		type[MAXTYPELEN+1];	/* Adapter type */
	char		configure[2];		/* Configure device? */
	unsigned short	dma1;			/* DMA channel 1 */
	unsigned short	dma2;			/* DMA channel 2 */
	unsigned short	ipl;			/* Interrupt priority */
	unsigned short	ivect;			/* Interrupt vector */
	unsigned short	ishare;			/* Interrupt sharing */
	unsigned long	sioaddr;		/* Start I/O addr */
	unsigned long	eioaddr;		/* End I/O addr */
	unsigned long	smemaddr;		/* Start memory addr */
	unsigned long	ememaddr;		/* End memory addr */
	/* This is derived */
	unsigned short	unit;			/* Unit number */
};


struct mdevline {
	char		name[MAXNAMELEN+1];	/* Name of driver */
	char		funclist[16+1];		/* Function list */
	char		characs[16+1];		/* Characteristics */
	char		prefix[MAXPREFIXLEN+1];	/* Driver prefix */
	char		bmajor[7];		/* Block major number */
	char		cmajor[7];		/* Character major number */
	unsigned short	minunits;		/* Minimum units */
	unsigned short	maxunits;		/* Maximum units */
	short		dma;			/* DMA channel */
};
#define	MDEVFIELDS	9


static char	*progname;


static void		fatalerror();
static struct diskdesc	*readinput();
static void		build_mdevice();
static void		update_mdevline();
static void		build_sdevice();
static void		build_HBA_tbl();
static void		build_DCD_tbl();

/* directory info */
char SDEVICEDIR[100]=		"/etc/conf/sdevice.d";
char SDISDEV[100]=		"/etc/conf/sdevice.d/sdi";
char SDISPACE[100]=		"/etc/conf/pack.d/sdi/space.c";
char SDIGEN[100]=		"/etc/conf/pack.d/sdi/space.gen";
char DCDSPACE[100]=		"/etc/conf/pack.d/dcd/space.c";
char DCDGEN[100]=		"/etc/conf/pack.d/dcd/space.gen";
char DCDSDEV[100]=		"/etc/conf/sdevice.d/dcd";
char DCDTMPL1[100]=		"/etc/conf/pack.d/dcd/t_athd.gen";
char DCDTMPL2[100]=		"/etc/conf/pack.d/dcd/t_mcesdi.gen";
char DCDTMPL3[100]=		"/etc/conf/pack.d/dcd/t_mcst.gen";
char MDEVICE[100]=		"/etc/conf/cf.d/mdevice";

char *p_index[9]= {"Primary", "", "", "",
		   "Secondary", "", "", "",
		   "3rd"};


main(argc, argv)
int	argc;
char	*argv[];
{
	struct diskdesc	*adapters;		/* Chain of adapter descriptions */

	/* Accept no arguments */
	if (argc > 2) {
		(void) fprintf(stderr, "Usage: %s [root]\n", argv[0]);
		return (EX_SYNTAX);
	}
	/* the 2nd parameter is for testing purpose only */
	/* and should NOT be used for non-testing purpose*/
	/* it substitute in the root directory		 */
	if (argc == 2) { /* which normally is your current test directory+tmp*/
		sprintf(SDEVICEDIR, "%s/etc/conf/sdevice.d", argv[1]);
		sprintf(SDISDEV,    "%s/etc/conf/sdevice.d/sdi", argv[1]);
		sprintf(SDISPACE,   "%s/etc/conf/pack.d/sdi/space.c", argv[1]);
		sprintf(SDIGEN,	    "%s/etc/conf/pack.d/sdi/space.gen",argv[1]);
		sprintf(DCDSPACE,   "%s/etc/conf/pack.d/dcd/space.c", argv[1]);
		sprintf(DCDGEN,	    "%s/etc/conf/pack.d/dcd/space.gen",argv[1]);
		sprintf(DCDSDEV,    "%s/etc/conf/sdevice.d/dcd", argv[1]);
		sprintf(DCDTMPL1,"%s/etc/conf/pack.d/dcd/t_athd.gen",argv[1]);
		sprintf(DCDTMPL2,"%s/etc/conf/pack.d/dcd/t_mcesdi.gen",argv[1]);
		sprintf(DCDTMPL3,"%s/etc/conf/pack.d/dcd/t_mcst.gen",argv[1]);
		sprintf(MDEVICE,"%s/etc/conf/cf.d/mdevice",argv[1]);
	}
	

	progname = argv[0];

	adapters = readinput();
	if (adapters == NULL)
		return (EX_SUCCESS);

	build_mdevice(adapters);
	build_sdevice(adapters);
	build_HBA_tbl(adapters);
	build_DCD_tbl(adapters);

	return (EX_SUCCESS);
}


/* VARARGS */
static void
fatalerror(retval, format, arg1, arg2, arg3)
int	retval;
char	*format;
int	arg1, arg2, arg3;
{
	(void) fprintf(stderr, "%s: ", progname);
	(void) fprintf(stderr, format, arg1, arg2, arg3);
	(void) fputc('\n', stderr);
	if (errno)
		perror(progname);
	exit(retval);
}


static struct diskdesc *
readinput()
{
	char		*ptr;			/* String pointer */
	unsigned short	lineno,			/* Input line number */
			unit;			/* Unit number */
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
			fatalerror(EX_FAILURE, "Insufficient memory");

		diskdesc->next = NULL;

		retval = scanf("%8s \"%64[^\"]\" %4s %1s %hu %hu %hu %hu %hu %lx %lx %lx %lx",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       &diskdesc->dma1, &diskdesc->dma2,
			       &diskdesc->ipl, &diskdesc->ivect,
			       &diskdesc->ishare,
			       &diskdesc->sioaddr, &diskdesc->eioaddr,
			       &diskdesc->smemaddr, &diskdesc->ememaddr);
		if (retval == EOF)
			return (adapters);

		if (retval != ARGSPERLINE)
			fatalerror(EX_BADARG, "Invalid field %d in line %u of input",
				   retval+1, lineno);

		for (ptr = diskdesc->type; *ptr != '\0'; ptr++)
			if (isupper(*ptr))
				*ptr = toupper(*ptr);
		if (strcmp(diskdesc->type, SCSITYPE) != 0 && strcmp(diskdesc->type, DCDTYPE) != 0)
			fatalerror(EX_BADARG, "Unrecognized adapter type %s in line %u of input",
				   diskdesc->type, lineno);

		if (strchr(VALIDCONF, diskdesc->configure[0]) == NULL)
			fatalerror(EX_BADARG, "Unrecognized configuration flag %c in line %u of input",
				   diskdesc->configure[0], lineno);
		if (islower(diskdesc->configure[0]))
			diskdesc->configure[0] = toupper(diskdesc->configure[0]);

		if (diskdesc->ishare < MINISHARE || diskdesc->ishare > MAXISHARE)
			fatalerror(EX_BADARG, "Unrecognized interrupt sharing code %u in line %u of input",
				   diskdesc->ishare, lineno);

		if (diskdesc->sioaddr > diskdesc->eioaddr)
			fatalerror(EX_BADARG, "Starting I/O address %x exceeds ending I/O address %x in line %u of input",
				   diskdesc->sioaddr, diskdesc->eioaddr, lineno);

		if (diskdesc->smemaddr > diskdesc->ememaddr)
			fatalerror(EX_BADARG, "Starting memory address %x exceeds ending memory address %x in line %u of input",
				   diskdesc->smemaddr, diskdesc->ememaddr, lineno);

		/* Check for duplicate devices preceding this one, and set the
		   unit number appropriately */
		unit = 1;
		if (diskdesc->configure[0] == 'Y') {
			for (tmpdesc = adapters; tmpdesc != NULL; tmpdesc = tmpdesc->next)
				if (tmpdesc->configure[0] == 'Y' &&
				    strcmp(tmpdesc->name, diskdesc->name) == 0)
					unit++;
		}
		diskdesc->unit = unit;

		*nextlink = diskdesc;			/* Add to end of chain */
		nextlink = &diskdesc->next;		/* Point to next link */
		lineno++;
	}
}


/*
 * Extract information from the mdevice file for each adapter.  Update
 * the DMA channel if needed.
 */
static void
build_mdevice(adapters)
struct diskdesc	*adapters;
{
	FILE		*fp;		/* mdevice file */
	unsigned short	found;		/* Found previous mdevice match? */
	int		retval;		/* Return value from fscanf */
	struct diskdesc	*diskdesc;	/* Description for searching */
	struct mdevline	*mdevline;	/* Line from mdevice file */

	if ((mdevline = (struct mdevline *) malloc(sizeof(struct mdevline))) == NULL)
		fatalerror(EX_FAILURE, "Insufficient memory");

	if ((fp = fopen(MDEVICE, "r")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for read", MDEVICE);

	for (;;) {
		retval = fscanf(fp, "%8s %16s %16s %6s %6s %6s %hu %hu %hd",
			       mdevline->name, mdevline->funclist,
			       mdevline->characs, mdevline->prefix,
			       mdevline->bmajor, mdevline->cmajor,
			       &mdevline->minunits, &mdevline->maxunits,
			       &mdevline->dma);
		if (retval == EOF)
			break;

		if (retval != MDEVFIELDS)
			fatalerror(EX_FAILURE, "Problem reading file %s", MDEVICE);

		if (mdevline->dma == -1)
			mdevline->dma = 0;

		found = 0;
		for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next)
			if (strcmp(diskdesc->name, mdevline->name) == 0) {
				found++;
				diskdesc->mdevline = mdevline;
			}

		if (found)
			if ((mdevline = (struct mdevline *) malloc(sizeof(struct mdevline))) == NULL)
				fatalerror(EX_FAILURE, "Insufficient memory");
	}

	free((char *) mdevline);

	if (fclose(fp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", MDEVICE);

	/* If the DMA field for an adapter has changed from the existing
	   mdevice file, update the mdevice file */
	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {
		/* If no mdevline at this point, configuration is screwed up */
		if (diskdesc->mdevline == NULL)
			fatalerror(EX_FAILURE, "No mdevice file entry for device %s", diskdesc->name);
		if (diskdesc->dma1 != diskdesc->mdevline->dma) {
			diskdesc->mdevline->dma = diskdesc->dma1;
			update_mdevline(diskdesc->mdevline);
		}
	}
}


/*
 * Update the mdevice file line for a specifid adapter
 */
static void
update_mdevline(mdevline)
struct mdevline	*mdevline;
{
	FILE	*fp;			/* Master file */
	short	dma;
	char	*tmpdir,
		buf[256];

	dma = mdevline->dma;
	if (dma == 0)
		dma = -1;

	tmpdir = tmpnam(NULL);
	if (mkdir(tmpdir, S_IRWXU) != 0)
		fatalerror(EX_FAILURE, "Cannot create temporary directory %s", tmpdir);

	(void) sprintf(buf, "%s/Master", tmpdir);
	if ((fp = fopen(buf, "w")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for write", buf);

	if (fprintf(fp, "%s	%s	%s	%s	%s	%s	%hu	%hu	%hd\n",
	       mdevline->name, mdevline->funclist,
	       mdevline->characs, mdevline->prefix,
	       mdevline->bmajor, mdevline->cmajor,
	       mdevline->minunits, mdevline->maxunits,
	       dma) < 0)
		fatalerror(EX_FAILURE, "Problem writing to file %s", buf);

	if (fclose(fp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", buf);

	(void) sprintf(buf, "cd %s; %s %s", tmpdir, IDINSTALL, mdevline->name);
	if (system(buf) == -1)
		fatalerror(EX_FAILURE, "Cannot fork process");

	if (remove(tmpdir) != 0)
		fatalerror(EX_FAILURE, "Cannot remove %s", tmpdir);
}


/*
 * Build all the sdevice files.  We build them for the following:
 *	sdi
 *	dcd
 *	each adapter
 */
static void
build_sdevice(adapters)
struct diskdesc	*adapters;
{
	char		sdevname[sizeof(SDEVICEDIR) + MAXNAMELEN + 1],
			*openmode;
	unsigned short	sdiunit,
			dcdunit;
	FILE		*sdifp,
			*dcdfp,
			*sdevfp;

	if ((sdifp = fopen(SDISDEV, "w")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for write", SDISDEV);
	if ((dcdfp = fopen(DCDSDEV, "w")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for write", DCDSDEV);
	sdiunit = dcdunit = 1; /* change from 0 to 1 */

	while (adapters != NULL) {
		(void) sprintf(sdevname, "%s/%s", SDEVICEDIR, adapters->name);

		if (adapters->unit == 1) /* change from 0 to 1 */
			openmode = "w";
		else
			openmode = "a";

		if ((sdevfp = fopen(sdevname, openmode)) == NULL) 
			fatalerror(EX_FAILURE, "Cannot open file %s for write", sdevname);

		if (fprintf(sdevfp, "%s	%c	%u	%u	0	0	%lx	%lx	%lx	%lx\n",
				    adapters->name, adapters->configure[0], adapters->unit,
				    adapters->ipl,
				    adapters->sioaddr, adapters->eioaddr,
				    adapters->smemaddr, adapters->ememaddr) < 0)
			fatalerror(EX_FAILURE, "Problem writing to file %s", sdevname);

		if (fclose(sdevfp) != 0)
			fatalerror(EX_FAILURE, "Problem closing file %s", sdevname);

		if (adapters->configure[0] == 'Y') {

			if (fprintf(sdifp, "sdi	%c	%u	%u	%u	%u	0	0	0	0\n",
				   	adapters->configure[0], sdiunit,
				   	adapters->ipl, adapters->ishare, adapters->ivect) < 0)
				fatalerror(EX_FAILURE, "Problem writing to file %s", SDISDEV);

			/* Direct coupled device? */
			if (strcmp(adapters->type, DCDTYPE) == 0) {
				if (fprintf(dcdfp, "dcd	%c	%u	0	0	0	0	0	0	0\n",
					    	adapters->configure[0], dcdunit) < 0)
					fatalerror(EX_FAILURE, "Problem writing to file %s", DCDSDEV);
				dcdunit++;
			}

			sdiunit++;

		}

		adapters = adapters->next;
	}

	/* If no direct coupled devices, make sure we turn off the dcd module */
	/* changed from 0 to 1 */
	if (dcdunit == 1)
		if (fprintf(dcdfp, "dcd	N	0	0	0	0	0	0	0	0\n") < 0)
			fatalerror(EX_FAILURE, "Problem writing to file %s", DCDSDEV);

	if (fclose(dcdfp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", DCDSDEV);
	if (fclose(sdifp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", SDISDEV);
}


/*
 * Build the HBA table in sdi/space.c
 */
static void
build_HBA_tbl(adapters)
struct diskdesc	*adapters;
{
	FILE	*spacefp,		/* sdi space.c file */
		*genfp;			/* sdi skeleton file for building space.c */
	size_t	inlen;			/* Length of input line */
	int	retval;			/* Return value from fscanf */
	char	buf[1024],		/* File input buffer */
		*match;			/* Matched substitution string */
	int     dcd_flag;		/* 1=dcd present else 0 */
	unsigned short 	dcd_int;	/* saves the dcd_interrupt */
	unsigned long   dcd_sioaddr; 	/* saves the dcd sioaddr */
        unsigned long   dcd_dma1;	/* saves the dcd dma */
        unsigned long   dcd_dma2;	/* saves the dcd dma */

	if ((spacefp = fopen(SDISPACE, "w")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for write", SDISPACE);

	if ((genfp = fopen(SDIGEN, "r")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for read", SDIGEN);

	for (;;) {
		retval = fscanf(genfp, "%1024[^\n]", buf);
		if (retval == EOF)
			break;
		(void) fgetc(genfp);	/* Skip newline */

		if (retval == 0)
			buf[0] = '\0';	/* Immediate new line */

		inlen = strlen(buf);
		if (inlen != 0)
			match = strstr(buf, SDIREPL);
		else
			match = NULL;

		if (match == NULL) {
			if (fprintf(spacefp, "%s\n", buf) < 0)
				fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
		}
		else {
			/* Found substitution token.  Write preceding characters. */
			if (fwrite(buf, sizeof(char), match - buf, spacefp) != match - buf)
				fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);

			/* Insert the configuration information */
			dcd_flag = 0;
			while (adapters != NULL) {

				if (adapters->configure[0] == 'Y') {
					if (strcmp(adapters->type, DCDTYPE) == 0)
						{
						dcd_int= 1 << (adapters->ivect-1);
						dcd_sioaddr=adapters->sioaddr;
						dcd_dma1=adapters->dma1;
						dcd_dma2=adapters->dma2;
						dcd_flag=1;
						adapters = adapters->next;
						continue;
						}
					if (fprintf(spacefp, "{\"%s\", 7, 0x%x, %sintr, %sfreeblk, %sgetblk,\n",
						    adapters->fullname,
						    1 << (adapters->ivect-1),
						    adapters->mdevline->prefix, adapters->mdevline->prefix,
						    adapters->mdevline->prefix) < 0)
						fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
					if (fprintf(spacefp, "	%sicmd, %sinit, %sname, %ssend, %sxlat, %sopen, %sclose,\n",
						    adapters->mdevline->prefix, adapters->mdevline->prefix,
						    adapters->mdevline->prefix, adapters->mdevline->prefix,
						    adapters->mdevline->prefix, adapters->mdevline->prefix,
						    adapters->mdevline->prefix) < 0)
						fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
					if (fprintf(spacefp, "	%sioctl, 0x%lx, 0, %u, %u, 0 },\n",
						    adapters->mdevline->prefix,
						    adapters->sioaddr,
						    adapters->dma1, adapters->dma2) < 0)
						fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
				}

				adapters = adapters->next;

			}
			
			if (dcd_flag==1) {
				if (fprintf(spacefp, 
"{\"DCD ISC HPDD\", 7, 0x%x, dcd_intr, dcd_freeblk, dcd_getblk,\n", dcd_int) < 0)
					fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
				if (fprintf(spacefp, 
"	dcd_icmd, dcd_init, dcd_name, dcd_send, dcd_xlat, dcd_open, dcd_close,\n") < 0)
					fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
				if (fprintf(spacefp, "	dcd_ioctl, 0x%lx, 0, %u, %u, 0 },\n", dcd_sioaddr, dcd_dma1, dcd_dma2) < 0)
					fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
			}

			/* Write trailing characters */
			if (fwrite(match + SDIREPLLEN, sizeof(char), inlen - SDIREPLLEN, spacefp) != inlen - SDIREPLLEN)
				fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
			if (fputc('\n', spacefp) == EOF)
				fatalerror(EX_FAILURE, "Problem writing to file %s", SDISPACE);
		}

	}

	if (fclose(genfp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", SDIGEN);
	if (fclose(spacefp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", SDISPACE);
}


static void
build_DCD_tbl(adapters)
struct diskdesc	*adapters;
{
	FILE	*spacefp,		/* dcd space.c file */
		*dcdfp1,		/* dcd template file for athd  */
		*dcdfp2,		/* dcd template file for mcesdi*/
		*dcdfp3,		/* dcd template file for mcst  */
		*genfp;			/* dcd skeleton file for space.c */
	size_t	inlen;			/* Length of input line */
	int	retval, retval1,
		retval2,retval3;	/* Return value from scanf */
	char	buf_gen[1024];		/* File input buffer */
	char	buf[3][2048];		/* File input buffer */
	char	*match;			/* Matched substitution string */
	int no_minor[4]={-4,-4,-4,-4};   /* used as prim/sec index and minor*/
	int type_no; /* 0=athd, 1=mcesdi, 2=mcst */
	unsigned long secondary=0;	/* secondary i/o address */

	if ((spacefp = fopen(DCDSPACE, "w")) == NULL) 
		fatalerror(EX_FAILURE,"Cannot open file %s for write",DCDSPACE);

	if ((genfp = fopen(DCDGEN, "r")) == NULL) 
		fatalerror(EX_FAILURE, "Cannot open file %s for read", DCDGEN);

	/* get t_athd ,t_mcesdi, t_mcst put buf1, buf2, buf3, respectively */
	retval1 = read_template(DCDTMPL1, buf[0]);
	retval2 = read_template(DCDTMPL2, buf[1]);
	retval3 = read_template(DCDTMPL3, buf[2]);

	for (;;) {
		retval = fscanf(genfp, "%1024[^\n]", buf_gen); /* read 1 line */
		if (retval == EOF)
			break;		/* if eof then I am done */
		(void) fgetc(genfp);	/* Skip newline */
		if (retval == 0)
			buf_gen[0] = '\0';	/* Immediate new line */
		inlen = strlen(buf_gen);
		if (inlen != 0)
			match = strstr(buf_gen, DCDREPL); /* look for key */
		else
			match = NULL;
		if (match == NULL) {
			if (fprintf(spacefp, "%s\n", buf_gen) < 0)
				fatalerror(EX_FAILURE, 
					"Problem writing to file %s", DCDSPACE);
		}
		else {
			/* Found substitution token.  Write 
			 * preceding characters. */
			if (fwrite(buf_gen, sizeof(char), match - buf_gen, 
					spacefp) != match - buf_gen)
				fatalerror(EX_FAILURE, 
					"Problem writing to file %s", DCDSPACE);

			/* Insert the configuration information */
			while (adapters != NULL) {
				if ((adapters->configure[0]=='Y') && 
					(strcmp(adapters->type,
						DCDTYPE)==NULL)){
					type_no=find_type(adapters->name);
					if (type_no==0)
						secondary=adapters->sioaddr +
							0x206;	
					no_minor[type_no]=no_minor[type_no]+4;
					fprintf(spacefp, buf[type_no], 
					      p_index[no_minor[type_no]],
					      adapters->fullname,
					      adapters->sioaddr,
					      secondary,
					      no_minor[type_no],
					      adapters->ivect);

				} /* end of configure[0] == 'Y' */
				adapters = adapters->next;
			} /* end of while adapter != NULL */

			/* Write trailing characters */
			if (fwrite(match + DCDREPLLEN, sizeof(char), 
				inlen - DCDREPLLEN, spacefp) != 
				inlen - DCDREPLLEN)
				fatalerror(EX_FAILURE,
					"Problem writing to file %s",DCDSPACE);
			if (fputc('\n', spacefp) == EOF)
				fatalerror(EX_FAILURE, 
					"Problem writing to file %s", DCDSPACE);
		} /* end of else NULL */

	} /* end of for(;;) */

	if (fclose(genfp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", DCDGEN);
	if (fclose(spacefp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", DCDSPACE);
}


/* 
 * 		read_template()
 * 
 * Description: open one of 3 $ROOT/ect/conf/pack.d/dcd/t_* template 
 *	        files for the space.c. It reads the file and returns number
 *		bytes read.
 */
read_template(file_name, buf)
char *file_name;
char *buf;
{
	FILE *fp;
	int retval, totval=0;

	if ((fp = fopen(file_name, "r")) == NULL) 
		fatalerror(EX_FAILURE,"Cannot open file %s for read",file_name);
	totval=fread(buf, sizeof(char), 2048, fp);
	if (fclose(fp) != 0)
		fatalerror(EX_FAILURE, "Problem closing file %s", file_name);
	return totval;
}

/*
 *				find_type()
 *
 */
find_type(adapter_name)
char *adapter_name;
{
	if (strcmp(adapter_name, "athd")   == NULL) return 0;
	if (strcmp(adapter_name, "mcesdi") == NULL) return 1;
	if (strcmp(adapter_name, "mcst")   == NULL) return 2;
}


