/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)libDtI:devtab.c	1.7"
#endif
/*
 *	This provides a library interface to the device table similar to
 *	the commands getdev and devattr; that functionality is heavily used
 *	in device administration, so that considerable overhead would be
 *	introduced by continually spawning child processes to get this data.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Memutil/memutil.h>

extern	char	*DtamGetTxt();

#define	FS	"\001"

#define	alias_disk		"dtdevtab:01" FS "Disk_%c"
#define	alias_ctape1		"dtdevtab:02" FS "Cartridge_Tape"

#define	tag_disk		"dtdevtab:03" FS "A"

#define	desc_disk3		"dtdevtab:04" FS "3.5 inch"
#define	desc_disk5		"dtdevtab:05" FS "5.25 inch"
#define	desc_disk0		"dtdevtab:06" FS "Floppy Disk Drive %c"
#define desc_ctape1		"dtdevtab:07" FS "Cartridge Tape Drive"

#define	TRUE	1
#define	FALSE	0

char	DEVTAB[]	= "/etc/device.tab";
char	ALIAS[]		= "alias";
char	DTALIAS[]	= "dtalias";
char	DESC[]		= "desc";
char	DTDESC[]	= "dtdesc";
char	CDEVICE[]	= "cdevice";
char	BDEVICE[]	= "bdevice";
char	CTAPE1[]	= "ctape1";
char	DISKETTE[]	= "diskette";

#define	FIRST	1
#define	NEXT	0

char	*devtab = NULL;
int	devfd;

static	int
FetchDevtab()
{
struct	stat	st_buf;
	char	*ptr;

	if (stat(DEVTAB, &st_buf) != -1
	&& (devfd = open(DEVTAB,O_RDONLY)) != -1) {
		devtab = mmap((caddr_t)0, st_buf.st_size, PROT_READ,
				MAP_SHARED, devfd, 0);
	}
	else
		devtab = (char *)-1;
	return (devtab != (char *)-1);
}

char *
DtamGetDev(char *pattern, int flag)
{
static	char	*nextptr = NULL;
	char	*ptr, *devline;

	if (devtab == NULL)
		if (!FetchDevtab())
			return	NULL;
	if (flag == FIRST || nextptr == NULL)
		nextptr = devtab;
	if ((ptr = strstr(nextptr, pattern)) == NULL) {
		nextptr = devtab;
		return NULL;
	}
	nextptr = strchr(ptr,'\n');
	while (*ptr != '\n')
		ptr--;
	if ((devline=(char *)MALLOC(nextptr-ptr+1)) == NULL)
		return NULL;
	ptr++;
	strncpy(devline, ptr, nextptr-ptr);
	devline[nextptr-ptr] = 0;
	return devline;
}

char *
DtamDevAttr(char *devline, char *pattern)
{
	char	*ptr = devline, *endstr, *attr = NULL;

	endstr = strchr(ptr, ':');
	if (strcmp(pattern, ALIAS) != 0) {
		ptr = endstr+1;
		endstr = strchr(ptr, ':');
		if (strcmp(pattern, CDEVICE)  != 0) {
			ptr = endstr+1;
			endstr = strchr(ptr, ':');
			if (strcmp(pattern, BDEVICE) != 0) {
				if ((ptr=strstr(endstr+1,pattern)) != NULL) {
					ptr += strlen(pattern);
					if (ptr[0] == '=' && ptr[1] == '"') {
						ptr += 2;
						endstr = strchr(ptr, '"');
					}
					else {
						ptr = NULL;
					}
				}
			}
		}
	}
	if (ptr == NULL || ptr == endstr)
		attr = NULL;
	else {
		if ((attr=(char *)MALLOC(endstr-ptr+2)) != NULL) {
			strncpy(attr, ptr, endstr-ptr);
			attr[endstr-ptr] = 0;
		}
	}
	return attr;
}

/*
 *	devalias is an "internationalization" of the /etc/device.tab alias
 *	In general, if a device entry has a dtalias attribute, this is read
 *	as a message catalog id, with optional following default text:
 *
 *		dtalias="<pathname>:<index>[:<text>]"
 *
 *	in the absence of a following text component, the alias attribute
 *	is taken as the default text argument to gettxt().
 *	If no dtalias attribute exists, and the device is one of the standard
 *	ones (diskette? and ctape1), it is special-cased through DtamGetTxt.
 *	In all other cases, devalias returns the alias.
 *
 *	AliasMap retains a trace of devalias calls, with the mapping of the
 *	returned value to the actual alias in the device line.
 */
char	**AliasMap = NULL;

static	char *
SetAliasMap(char *i18nalias, char *devline)
{
static	int	cnt = 0;

	if (cnt++)
		AliasMap = (char **)REALLOC(AliasMap,(2*cnt+1)*sizeof(char *));
	else
		AliasMap = (char **)MALLOC(3*sizeof(char *));
	if (AliasMap == NULL) {
		fprintf(stderr, "cannot allocate device alias table\n");
		return NULL;
	}
	AliasMap[2*cnt-2] = STRDUP(i18nalias);
	if (strncmp(devline, i18nalias, strlen(i18nalias))==0)
		AliasMap[2*cnt-1] = AliasMap[2*cnt-2];
	else
		AliasMap[2*cnt-1] = DtamDevAttr(devline,ALIAS);
	AliasMap[2*cnt] = NULL;
	return i18nalias;
}

char *
DtamMapAlias(char *i18nalias)	/* return /etc/device.tab alias */
{
	int	n;

	for (n = 0;  AliasMap[n]; n += 2) {
		if (strcmp(AliasMap[n], i18nalias) == 0)
			return AliasMap[n+1];
	}
	return i18nalias;
}

char *
DtamDevAlias(char *devline)
{
	char	*ptr, buf[40];

	if (ptr = DtamDevAttr(devline, DTALIAS)) {
		char	*p1, *p2;

		if ((p1 = strchr(ptr,':')) == NULL) {
			fprintf(stderr,"invalid %s in %s\n", DTALIAS, DEVTAB);
			strcpy(buf, p2=DtamDevAttr(devline, ALIAS));
			FREE(p2);
		}
		else if ((p2 = strrchr(ptr,':')) == p1) {
			strcpy(buf, gettxt(ptr, p2=DtamDevAttr(devline,ALIAS)));
			FREE(p2);
		}
		else {
			*p2++ = '\0';
			strcpy(buf, gettxt(ptr, p2));
		}
		FREE(ptr);
		return SetAliasMap(STRDUP(buf), devline);
	}
	else if (strncmp(devline, CTAPE1, sizeof(CTAPE1)-1)==0) {
		return SetAliasMap(STRDUP(DtamGetTxt(alias_ctape1)),devline);
	}
	else if (strncmp(devline, DISKETTE, sizeof(DISKETTE)-1)==0) {
		/*
		 *	translate "diskette1" etc. to "Disk_A" etc
		 *	(where the etc. means that one sequence is
		 *	mapped to another, starting with the I18N
		 *	tag_disk character.
		 */
		char	c = devline[sizeof(DISKETTE)-1];
		char	A = *DtamGetTxt(tag_disk);
		sprintf(buf, DtamGetTxt(alias_disk), c + A - '1');
		return SetAliasMap(STRDUP(buf), devline);
	}
	else
		return SetAliasMap(DtamDevAttr(devline, ALIAS), devline);
}

/*
 *	devdesc is similar to devalias, keying off a dtdesc attribute instead
 */
char *
DtamDevDesc(char *devline)
{
	char	*ptr, *p1, *p2, buf[80];

	if (ptr = DtamDevAttr(devline, DTDESC)) {
		if ((p1 = strchr(ptr,':')) == NULL) {
			fprintf(stderr,"invalid %s in %s\n", DTDESC, DEVTAB);
			strcpy(buf, p2=DtamDevAttr(devline, DESC));
			FREE(p2);
		}
		else if ((p2 = strrchr(ptr,':')) == p1) {
			strcpy(buf, gettxt(ptr, p2=DtamDevAttr(devline, DESC)));
			FREE(p2);
		}
		else {
			*p2++ = '\0';
			strcpy(buf, gettxt(ptr, p2));
		}
		FREE(ptr);
	}
	else if (strncmp(devline, CTAPE1, sizeof(CTAPE1)-1)==0) {
		strcpy(buf, DtamGetTxt(desc_ctape1));
	}
	else if (strncmp(devline, DISKETTE, sizeof(DISKETTE)-1)==0) {
		if ((p1 = DtamDevAttr(devline,"fmtcmd")) != NULL
		&&  (p2 = strpbrk(p1,"35")) != NULL) {
			sprintf(buf, DtamGetTxt(*p2=='3'? desc_disk3:
							  desc_disk5));
			FREE(p1);
		}
		else
			sprintf(buf, DtamGetTxt(desc_disk0),
					devline[sizeof(DISKETTE)]);
	}
	else
		return DtamDevAttr(devline, DESC);
	return STRDUP(buf);
}

/*
 *	the following is a convenience routine for the Finder
 */
char *
DtamGetAlias(char *pattern, int flag)
{
	char	*dev, *ptr;

	if (ptr=DtamGetDev(pattern,flag)) {
		dev = ptr;
		ptr = DtamDevAlias(dev);
		FREE(dev);
	}
	return ptr;
}
