/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtm:fn_devtab.c	1.1"
/*
 *	This provides a library interface to the device table similar to
 *	the commands getdev and devattr; that functionality is heavily used
 *	in device administration, so that considerable overhead would be
 *	introduced by continually spawning child processes to get this data.
 */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define	TRUE	1
#define	FALSE	0

#define	DEVICE_TABLE	"/etc/device.tab"

#define	FIRST	1
#define	NEXT	0

char	*devtab = NULL;
int	devfd;

int	FetchDevtab()
{
struct	stat	st_buf;
	char	*ptr;

	if (stat(DEVICE_TABLE, &st_buf) != -1
	&& (devfd = open(DEVICE_TABLE,O_RDONLY)) != -1) {
		devtab = mmap((caddr_t)0, st_buf.st_size, PROT_READ,
				MAP_SHARED, devfd, 0);
	}
	else
		devtab = (char *)-1;
	return (devtab != (char *)-1);
}

char	*getdev(pattern, flag)
	char	*pattern;
	int	flag;
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
	if ((devline=(char *)malloc(nextptr-ptr)) == NULL)
		return NULL;
	ptr++;
	strncpy(devline, ptr, nextptr-ptr);
	devline[nextptr-ptr] = 0;
	return devline;
}

char	*devattr(devline, pattern)
	char	*devline, *pattern;
{
	char	*ptr = devline, *endstr, *attr = NULL;

	endstr = strchr(ptr, ':');
	if (strcmp(pattern,"alias") != 0) {
		ptr = endstr+1;
		endstr = strchr(ptr, ':');
		if (strcmp(pattern,"cdevice")  != 0) {
			ptr = endstr+1;
			endstr = strchr(ptr, ':');
			if (strcmp(pattern, "bdevice") != 0) {
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
	if (ptr != NULL) {
		if ((attr=(char *)malloc(endstr-ptr+1)) != NULL) {
			strncpy(attr, ptr, endstr-ptr);
			attr[endstr-ptr] = 0;
		}
	}
	return attr;
}

char	*devdesc(devline)
	char	*devline;
{
	char	*ptr;

	if ((ptr = devattr(devline,"mdensdefault")) == NULL)
		ptr = devattr(devline,"desc");
	else {
		char	*densdev = getdev(ptr, NEXT);
		free(ptr);
		ptr = devattr(densdev,"desc");
		free(densdev);
	}
	return ptr;
}
