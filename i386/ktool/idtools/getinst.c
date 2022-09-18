/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/getinst.c	1.12"
#ident	"$Header:"

/* Get an entry from and ID configuration data file.
 *
 * The entry is
 * name = FIRST: first entry.
 * name = NEXT: next entry.
 * name = RESET: reset file pointer to beginning of file.
 * name = device/parameter name.
 *
 * Stp points to an mdev, mtun, sdev, stun, sasn stucture.
 *
 * Return  0 if EOF or cannot locate device.
 *	   1 if success.
 *	   IERR_OPEN if cannot open file.
 *	   IERR_READ if a read error occurs.
 *	   IERR_NFLDS if wrong number of fields.
 *	   IERR_FLAGS if incorrect flags field.
 *	   IERR_MAJOR if syntax error in major number(s).
 *	   IERR_MMRANGE if invalid multiple major range.
 *	   IERR_BCTYPE if invalid block/char type.
 *	   IERR_ENTRY if invalid entry-point name.
 *	   IERR_DEPEND if invalid dependee module name.
 *	   IERR_LOAD if invalid loadable module name.
 *
 * Calling functions can tell getinst the location of the files
 * by filling in values for the variables:
 *
 *	extern char instroot[], pathinst[];
 *
 * instroot[] is prepended to the directory name for directory forms
 *	(e.g. "mdevice.d").
 * pathinst[] is prepended to the file name for flat-file forms
 *	(e.g. "mdevice"); this is by default the "cf.d" subdirectory of
 *	the instroot directory.
 */

#include "inst.h"
#include "devconf.h"
#include "mdep.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

extern char linebuf[];	/* current input line */
extern char errbuf[];	/* buffer for error message */
char pathinst[256] = CFPATH;  /* directory containing Master and System files */
char instroot[256] = ROOT; /* configuration "root" directory (/etc/conf) */
int ignore_entries;	/* flag: ignore $entry lines in Master file */
int noloadable;	/* flag: ignore all the dynamic loadable entries in Master file */
char path[LINESZ];	/* path to the file being processed */

/* File type definitions */
ftype_t ftypes[] = {
/* MDEV */	{ "Master", "mdevice", 0, MDEV, MDEV_VER },
/* MDEV_D */	{ "Master", "mdevice.d", 1, MDEV, MDEV_VER },
/* SDEV */	{ "System", "sdevice", 0, SDEV, SDEV_VER },
/* SDEV_D */	{ "System", "sdevice.d", 1, SDEV, SDEV_VER },
/* MTUN */	{ "Mtune", "mtune", 0, MTUN, MTUNE_VER },
/* MTUN_D */	{ "Mtune", "mtune.d", 1, MTUN, MTUNE_VER },
/* STUN */	{ "Stune", "stune", 0, STUN, STUNE_VER },
/* STUN_D */	{ "Stune", "stune.d", 1, STUN, STUNE_VER },
/* SASN */	{ "Sassign", "sassign", 0, SASN, SASSIGN_VER },
/* SASN_D */	{ "Sassign", "sassign.d", 1, SASN, SASSIGN_VER },
/* NODE */	{ "Node", "node", 0, NODE, NODE_VER },
/* NODE_D */	{ "Node", "node.d", 1, NODE, NODE_VER },
		{ NULL }
};

driver_t *driver_info;

extern int getmajors();
#if !defined(__STDC__)
extern long strtol();
#endif
static int line(), same(), legalflags();


int
rdinst(type, buf, stp, prev_stat)
short type;
char *buf;
char *stp;
int prev_stat;
{
	struct multmaj mm;	/* used for multiple majors */
	struct mdev *mdev;
	struct sdev *sdev;
	struct mtune *mtune;
	struct stune *stune;
	struct sassign *sassign;
	struct node *node;
	char fmt[78];
	char dummy[2];
	char tag[12];
	int n;

	switch (type) {
	case MDEV:
		mdev = (struct mdev *)stp;
		if (prev_stat != I_MORE) {
			mdev->modtype[0] = '\0';
			mdev->depends = NULL;
			mdev->entries = NULL;
		}
		if (strncmp(buf, "$entry", 6) == 0 && isspace(buf[6])) {
			char *ename, old_c;

			if (ignore_entries)
				return I_MORE;
			for (buf += 7; isspace(*buf); buf++)
				;
			while (*buf != '\0') {
				ename = buf;
				while (*++buf != '\0' && !isspace(*buf))
					;
				if ((old_c = *buf) != '\0')
					*buf++ = '\0';
				n = lookup_entry(ename, &mdev->entries, 1);
				if (old_c != '\0')
					buf[-1] = old_c;
				if (!n)
					return IERR_ENTRY;
				for(; isspace(*buf); buf++)
					;
			}
			return I_MORE;
		}
		if (strncmp(buf, "$modtype", 8) == 0 && isspace(buf[8])) {
			int typelen;

			if (noloadable)
				return I_MORE;
			for (buf += 9; isspace(*buf); buf++)
				;
			if ((typelen = strlen(buf)) >= TYPESZ)
				return IERR_TYPE;
			buf[typelen - 1] = '\0'; /* drop the final newline */
			strcpy(mdev->modtype, buf);
			return I_MORE;
		}
		if (strncmp(buf, "$depend", 7) == 0 && isspace(buf[7])) {
			char *dname, old_c;
			struct depend_list *dep;

			if (noloadable)
				return I_MORE;
			for (buf += 8; isspace(*buf); buf++)
				;
			while (*buf != '\0') {
				dname = buf;
				while (*++buf != '\0' && !isspace(*buf))
					;
				if ((old_c = *buf) != '\0')
					*buf++ = '\0';
				if (strlen(dname) >= NAMESZ)
					return IERR_DEPEND;
				dep = (struct depend_list *)malloc(sizeof(struct depend_list));
				strcpy(dep->name, dname);
				dep->next = mdev->depends;
				mdev->depends = dep;
				if (old_c != '\0')
					buf[-1] = old_c;
				for(; isspace(*buf); buf++)
					;
			}
			return I_MORE;
		}
		sprintf(fmt, "%%%ds %%%ds %%%ds %%hd %%%ds %%%ds %%1s",
			NAMESZ - 1, PFXSZ - 1, FLAGSZ - 1,
			RANGESZ - 1, RANGESZ - 1);
		n = sscanf(buf, fmt,
				mdev->name,
				mdev->prefix,
				mdev->mflags,
				&mdev->order,
				mm.brange,
				mm.crange,
				dummy);
		if (n != 6)
			return IERR_NFLDS;

		/* make sure specified flags are all legal */
		if (!legalflags(mdev->mflags, ALL_MFLAGS))
			return IERR_FLAGS;
		if (INSTRING(mdev->mflags, FCOMPAT) &&
			!INSTRING(mdev->mflags, COMPAT))
			return IERR_FLAGS;

		/* convert the major numbers, read as a string         */
		/* into a number (if single major) or a start/end pair */
		/* if multiple majors are specified.                   */

		n = getmajors(mm.brange, &mdev->blk_start, &mdev->blk_end);
		if (n != 0)
			return n;

		n = getmajors(mm.crange, &mdev->chr_start, &mdev->chr_end);
		if (n != 0)
			return n;

		return 1;

	case SDEV:
		if (strncmp(buf, "$loadable", 9) == 0 && isspace(buf[9])) {
			char *mname;
			driver_t *drv;

			if (noloadable)
				return I_MORE;
			for (buf += 9; isspace(*buf); buf++)
				;
			mname = buf;
			while (*++buf != '\0' && !isspace(*buf))
					;
			if (*buf != '\0')
				*buf = '\0';
			if (strlen(mname) >= NAMESZ ||
				(drv = mfind(mname)) == NULL)
				return IERR_LOAD;
			drv->loadable++;
			return I_MORE;
		}
		sdev = (struct sdev *)stp;
		sprintf(fmt, "%%%ds", NAMESZ - 1);
		strcat(fmt, " %c %hd %hd %hd %hd %lx %lx %lx %lx %hd %1s");
		n = sscanf(buf, fmt,
				sdev->name,
				&sdev->conf,
				&sdev->units,
				&sdev->ipl,
				&sdev->itype,
				&sdev->vector,
				&sdev->sioa,
				&sdev->eioa,
				&sdev->scma,
				&sdev->ecma,
				&sdev->dmachan,
				dummy);
		if (n != 11)
			return IERR_NFLDS;
		return 1;

	case MTUN:
		mtune = (struct mtune *)stp;
#if defined(__STDC__)
		sprintf(fmt, "%%%ds", TUNESZ - 1);
		strcat(fmt, " %li %li %li %11s %1s");
		n = sscanf(buf, fmt,
				mtune->name,
				&mtune->def,
				&mtune->min,
				&mtune->max,
				tag,
				dummy);
		if (n >= 5 && strcmp(tag, "%%INS%%") == 0)
			n--;
		if (n != 4)
			return IERR_NFLDS;
#else	/* __STDC__ */
		{ char s1[16], s2[16], s3[16];
			sprintf(fmt, "%%%ds", TUNESZ - 1);
			strcat(fmt, " %16s %16s %16s %11s %1s");
			n = sscanf(buf, fmt,
					mtune->name,
					s1, s2, s3,
					tag,
					dummy);
			if (n >= 5 && strcmp(tag, "%%INS%%") == 0)
				n--;
			if (n != 4)
				return IERR_NFLDS;
			mtune->def = strtol(s1, NULL, 0);
			mtune->min = strtol(s2, NULL, 0);
			mtune->max = strtol(s3, NULL, 0);
		}
#endif	/* __STDC__ */
		return 1;

	case STUN:
		stune = (struct stune *)stp;
#if defined(__STDC__)
		sprintf(fmt, "%%%ds", TUNESZ - 1);
		strcat(fmt, " %li %1s");
		n = sscanf(buf, fmt,
				stune->name,
				&stune->value,
				dummy);
		if (n != 2)
			return IERR_NFLDS;
#else	/* __STDC__ */
		{ char s1[16];
			sprintf(fmt, "%%%ds", TUNESZ - 1);
			strcat(fmt, " %16s %1s");
			n = sscanf(buf, fmt,
					stune->name,
					s1,
					dummy);
			if (n != 2)
				return IERR_NFLDS;
			stune->value = strtol(s1, NULL, 0);
		}
#endif	/* __STDC__ */
		return 1;

	case SASN:
		sassign = (struct sassign *)stp;
		sassign->objname[0] = '\0';
		sassign->low = sassign->blocks = 0;
		sprintf(fmt, "%%%ds %%%ds %%ld %%%ds %%ld %%ld %%1s",
			NAMESZ - 1,
			NAMESZ - 1,
			MAXOBJNAME - 1);
		n = sscanf(buf, fmt,
				sassign->device,
				sassign->major,
				&sassign->minor,
				sassign->objname,
				&sassign->low,
				&sassign->blocks,
				dummy);
		if (n < 3 || n > 6)
			return IERR_NFLDS;
		return 1;

	case NODE:
		node = (struct node *)stp;
		sprintf(fmt, "%%%ds %%%ds %%10s %%%ds %%ld %%ld %%o %%ld %%1s",
			NAMESZ - 1,
			MAXOBJNAME - 1,
			NAMESZ - 1);
		n = sscanf(buf, fmt,
				node->major,
				node->nodename,
				tag,
				node->majminor,
				&node->uid,
				&node->gid,
				&node->mode,
				&node->level,
				dummy);

		switch (n) {
		case 4:
			node->uid = node->gid = -1;
			node->mode = 0666;
		case 7:
			node->level = 0;
			break;
		case 8:
			break;
		default:
			return IERR_NFLDS;
		}

		node->type = tag[0];
		if (node->type != BLOCK && node->type != CHAR)
			return IERR_BCTYPE;
		if (tag[1] == ':')
			node->maj_off = atoi(tag + 2);
		else
			node->maj_off = 0;
		if (isdigit(node->majminor[0])) {
			node->minor = atoi(node->majminor);
			node->majminor[0] = '\0';
		}
		return 1;
	}

	return IERR_READ;	/* shouldn't happen */
}


int
getinst(type, name, stp)
short type;
char *name;
char *stp;
{
	ftype_t	*ftp = &ftypes[type];
	int	stat;

	if (ftp->is_dir) {
		if (ftp->dp == NULL) {
			sprintf(path, "%s/%s", instroot, ftp->fname);
			if ((ftp->dp = opendir(path)) == NULL)
				return IERR_OPEN;
		}
		if (*name != *NEXT) {
			if (ftp->fp != NULL) {
				fclose(ftp->fp);
				ftp->fp = NULL;
			}
			rewinddir(ftp->dp);
			if (*name == *RESET)
				return 1;
		}
	}

	for (;;) {
		stat = getinstf(ftp, name, stp);
		if (stat != 0 || !ftp->is_dir)
			break;
		fclose(ftp->fp);
		ftp->fp = NULL;
	}

	return stat == 2 ? 0 : stat;
}
	

int
getinstf(ftp, name, stp)
ftype_t *ftp;
char *name;
char *stp;
{
	struct dirent *direntp;
	int state;
	struct stat fstat;

	if (ftp->fp == NULL) {
		if (ftp->is_dir) {
			while ((direntp = readdir(ftp->dp)) != NULL &&
			       direntp->d_name[0] == '.')
				;
			if (direntp == NULL)
				return 2;
			sprintf(path, "%s/%s/%s", instroot, ftp->fname,
				direntp->d_name);
		} else
			sprintf(path, "%s/%s", pathinst, ftp->fname);

		if (stat(path, &fstat) != 0)
			return IERR_STAT;
		if ((fstat.st_mode & S_IFMT) != S_IFREG)
			return IERR_NREG;

		if ((ftp->fp = fopen(path, "r")) == NULL)
			return IERR_OPEN;

		/* New file version defaults to 0 */
		ftp->ver = 0;
	}

	if (*name != *NEXT) {
		fseek(ftp->fp, 0L, 0);
		if (*name == *RESET)
			return 1;
	}

	state = 0;
	do {
next_line:
		if (line(ftp->fp) == 0)
			return ferror(ftp->fp)? IERR_READ : 0;

		/* Check if version number specified */
		if (strncmp(linebuf, "$version", 8) == 0 &&
		    isspace(linebuf[8])) {
			ftp->ver = atoi(linebuf + 9);
			/* Make sure this file is the right version */
			if (ftp->ver != ftp->cur_ver)
				return IERR_VER;

			goto next_line;
		}

		/*
		 * If we're searching for a particular name,
		 * keep going until we find it.
		 */
		if (*name != *FIRST && *name != *NEXT && !same(name))
			goto next_line;

		state = rdinst(ftp->basetype, linebuf, stp, state);

	} while (state == I_MORE);

	return state;
}


insterrmsg(errcode, ftype, dev)
int errcode;
int ftype;
char *dev;
{
	char	*filename = ftypes[ftype].lname;

	fprintf(stderr, "FILE: %s\n", path);
	switch (errcode) {
	case IERR_VER:
		sprintf(errbuf, EMSG_VER, filename);
		break;
	case IERR_NFLDS:
		sprintf(errbuf, EMSG_NFLDS, filename);
		break;
	case IERR_FLAGS:
		sprintf(errbuf, EMSG_FLAGS, dev, filename);
		break;
	case IERR_MAJOR:
		sprintf(errbuf, EMSG_MAJOR, dev, filename);
		break;
	case IERR_MMRANGE:
		sprintf(errbuf, EMSG_MMRANGE, dev, filename);
		break;
	case IERR_OPEN:
		sprintf(errbuf, EMSG_OPEN, filename);
		break;
	case IERR_BCTYPE:
		sprintf(errbuf, EMSG_BCTYPE, filename);
		break;
	case IERR_ENTRY:
		sprintf(errbuf, EMSG_ENTRY);
		break;
	case IERR_DEPEND:
		sprintf(errbuf, EMSG_DEPEND);
		break;
	case IERR_LOAD:
		sprintf(errbuf, EMSG_LOAD);
		break;
	case IERR_TYPE:
		sprintf(errbuf, EMSG_TYPE);
		break;
	case IERR_STAT:
		sprintf(errbuf, EMSG_STAT);
		break;
	case IERR_NREG:
		sprintf(errbuf, EMSG_NREG);
		break;
	default:
		sprintf(errbuf, EMSG_READ, filename);
	}
}


/* Read a line. Skip lines beginning with '*' or '#', and blank lines.
 * Return 0 if EOF. Return 1 otherwise.
 */
static int
line(fp)
FILE *fp;
{
	for (;;) {
		if (fgets(linebuf, LINESZ, fp) == NULL) {
			linebuf[0] = '\0';
			return(0);
		}
		if (*linebuf != '*' && *linebuf != '#' && *linebuf != '\n')
			return(1);
	}
}

/* Check if 'name' is the same string that begins in column 1 of 'linebuf'.
 * 'Name' must be null terminated. The first field of 'linebuf' which is being
 * compared must be followed by white space (this doesn't include '\0').
 * Return 1 if field 1 of 'linebuf' matches name, and 0 otherwise.
 */
static int
same(name)
char *name;
{
	char *b;

	for (b = linebuf; !isspace(*b) && *name != '\0'; b++, name++)
		if (*b != *name)
			return(0);
	if (isspace(*b) && *name == '\0')
		return(1);
	return(0);
}

/* Check for illegal flag characters.
 * Return 1 if all flags are OK, else 0.
 */
static int
legalflags(flags, legal)
char *flags;
char *legal;
{
	while (*flags) {
		if (!INSTRING(legal, *flags++))
			return 0;
	}
	return 1;
}


/* write mdev structure to file */

wrtmdev(d, fp)
struct mdev *d;
FILE *fp;
{
	struct multmaj mm;

	if (d->blk_start == d->blk_end)
		sprintf(mm.brange, "%hd", d->blk_start);
	else
		sprintf(mm.brange, "%hd-%hd", d->blk_start, d->blk_end);
	if (d->chr_start == d->chr_end)
		sprintf(mm.crange, "%hd", d->chr_start);
	else
		sprintf(mm.crange, "%hd-%hd", d->chr_start, d->chr_end);

	fprintf(fp, "%s\t%s\t%s\t%hd\t%s\t%s\n",
		d->name, d->prefix, d->mflags, d->order,
		mm.brange, mm.crange);
}

/* This routine is used to search through the Master table for
 * some specified device.  If the device is found, we return a pointer to
 * the device.  If the device is not found, we return a NULL.
 */
driver_t *
mfind(device)
char *device;
{
        register driver_t *drv;

        for (drv = driver_info; drv != NULL; drv = drv->next) {
                if (equal(device, drv->mdev.name))
                        return(drv);
        }
        return(NULL);
}

