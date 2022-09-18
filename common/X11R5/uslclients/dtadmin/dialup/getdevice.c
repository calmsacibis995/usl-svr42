/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/getdevice.c	1.20"
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLook.h>
#include <libDtI/DtI.h>
#include "uucp.h"
#include "error.h"

extern Boolean		getline();
extern void		CheckMode();

void FreeFileClass();
void FreeObject();
void FreeDeviceData();
void PutContainerItems();

static		exists;		/* Non-zero if file exists */
static		readable;	/* Non-zero if file is readable */
static		writeable;	/* Non-zero if file is writable or */
				/* directory is writable and file */
				/* doesn't exist */

static char	*lbuf;		/* Buffer for comments and none support types */
DmFclassPtr	acu_fcp, dir_fcp;

void
AddObjectToContainer(op)
DmObjectPtr op;
{
	/* add it to the end of list */
	if (df->cp->op == NULL)
		df->cp->op = op;
	else {
		DmObjectPtr endp;

		for (endp=df->cp->op; endp->next; endp=endp->next) ;
		endp->next = op;
	}
	df->cp->num_objs++;
}

void
DeleteContainerItems()
{
	register DmObjectPtr op = df->cp->op;

	for (; op->next; op=op->next)
		FreeObject (op);
	FreeFileClass (acu_fcp);
	FreeFileClass (dir_fcp);
	free (df->itp);
	if (df->select_op)
		free (df->select_op);
	free (df->cp);
}

void
DelObjectFromContainer(target_op)
DmObjectPtr target_op;
{
	register DmObjectPtr op = df->cp->op;

	if (op == target_op) {
		df->cp->op = op->next;
		df->cp->num_objs--;
	}
	else
		for (; op->next; op=op->next)
			if (op->next == target_op) {
				op->next = target_op->next;
				df->cp->num_objs--;
				break;
			}
	FreeObject(target_op);
}

DmFclassPtr
new_fileclass(type)
char *type;
{
	static char *iconpath;
        DmFclassPtr fcp;
	char icon[128];

        if ((fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec))) == NULL)
                return(NULL);

	if (!strcmp(type, ACU))
		sprintf(icon, "%s", ACU_ICON);
	else
		sprintf(icon, "%s", DIR_ICON);
	fcp->glyph = DmGetPixmap(SCREEN, icon);
        return(fcp);
}

extern DmObjectPtr
new_object(name,  pDeviceData)
char *name;
DeviceData  *pDeviceData;
{
        DmObjectPtr op, objp;
	Dimension width;
	static x, y;
	DeviceData * tap;

	if (df->cp->op != NULL) { /* already allocated */
		for (objp=df->cp->op; objp; objp=objp->next)
			if (!strcmp(objp->name, name)) {
				/* duplicate */
				tap = (DeviceData *)objp->objectdata;
				free(tap->portSpeed);
				free(tap->holdPortSpeed);
				tap->portSpeed = strdup("Any");
				tap->holdPortSpeed = strdup("Any");
				return((DmObjectPtr)OL_NO_ITEM);
			}
	}

        if (!(op = (DmObjectPtr)calloc(1, sizeof(DmObjectRec))))
                return(NULL);

        op->container = df->cp;
        if (name)
                op->name = strdup(name);

	op->x = op->y = UNSPECIFIED_POS;
	op->objectdata = pDeviceData;
        return(op);
} /* new_object */

void
new_container(path)
char *path;
{
        if ((df->cp = (DmContainerPtr)calloc(1, sizeof(DmContainerRec))) == NULL) {
#ifdef debug
                fprintf(stderr,"new_container: can not allocate memory\n");
#endif
		perror("calloc");
                return;
        }

        df->cp->path = strdup(path);
        df->cp->count = 1;
        df->cp->num_objs = 0;
}

void
GetContainerItems(path)
char *path;
{
	DeviceData  *tmp;
	DmObjectPtr	op;
	extern DmFclassPtr	acu_fcp, dir_fcp;
	extern DmObjectPtr new_object();
	struct stat	statbuf;
	int		na;
	char		*dev[D_ARG+2], buf[BUFSIZ];
	char		port[10];
	char		name[10];
	char		text[BUFSIZ];
	char		*convert();
	FILE *fdevice, *fopen();
	static unsigned	stat_flags = 0;	/* Mask returned by CheckMode() */

	CheckMode(path, &stat_flags);	/* get the file mode	*/
	exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
	readable = BIT_IS_SET(stat_flags, FILE_READABLE);
	writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
#ifdef debug
	fprintf(stderr, "/etc/uucp/Devices exists = %d, readable = %d, writeable = %d\n",
			exists, readable, writeable);
#endif
	if (!exists & !writeable) { /* somthing's serious wrong */
#ifdef debug
		fprintf(stderr, GGT(string_createFail), path);
		exit(1);
#endif
		sprintf(text, GGT(string_noFile), path);
		rexit(1, text, "");
	} else
	if (!readable & exists) {
#ifdef debug
		fprintf(stderr, GGT(string_accessFail), path);
		exit(2);
#endif
		sprintf(text, GGT(string_accessFail), path);
		rexit(2, text, "");
	} else
	if (exists & readable & !writeable) {
#ifdef debug
		fprintf(stderr, GGT(string_writeFail), path);
#endif
	}
	acu_fcp = new_fileclass(ACU);
	dir_fcp = new_fileclass(DIR);

	new_container(path);

	if ((fdevice = fopen(path, "r")) == NULL) {
		/* it's ok not able to open it since we knew that
		 * the file may not exist and we have the write privilege
		 * anyway.
		 */
#ifdef debug
		fprintf(stderr, GGT(string_openFail), path);
#endif
		;
	} else {
		stat(path, &statbuf);
		lbuf = malloc (statbuf.st_size + 1);
		lbuf[0] = '\0';
		while (getline(buf, BUFSIZ - 1, fdevice)) {
                    na = getargs(buf, dev, D_ARG);
		    if ( na < D_ARG ) continue;
                    if ( strcmp(dev[D_TYPE], ACU) != 0 &&
			 strcmp(dev[D_TYPE], DIR) != 0 &&
			 strcmp(dev[D_TYPE], DK ) != 0 ) {
			    sprintf (text,"%s %s %s %s %s %s\n",
					dev[0],
					dev[1],
					dev[2],
					dev[3],
					dev[4],
					dev[5]);
			    strcat (lbuf, text);
			    continue;
		    }
                    if ( strncmp(dev[D_LINE],"/dev/",5) == 0 ) {
                            /* strips off leading "/dev/" */
                            strcpy(dev[D_LINE], &(dev[D_LINE][5]) );
                    }
                    if ( strncmp(dev[D_LINE],"term/",5) == 0 ) {
                            /* strips off leading "term/" */
                            strcpy(dev[D_LINE], &(dev[D_LINE][5]) );
                    }
		    if (!strcmp(dev[D_LINE], "tty00") ||
			!strcmp(dev[D_LINE], "00s") ||
			!strcmp(dev[D_LINE], "00h") ||
			!strcmp(dev[D_LINE], "00"))
			    strcpy(port, "com1");
		    else if (!strcmp(dev[D_LINE], "tty01") ||
			     !strcmp(dev[D_LINE], "01s") ||
			     !strcmp(dev[D_LINE], "01h") ||
			     !strcmp(dev[D_LINE], "01"))
			    strcpy(port, "com2");
		    else
			    strcpy(port, dev[D_LINE]);
		
		    strcpy(name, port);
		    tmp = (DeviceData *) calloc(1, sizeof(DeviceData));
        	    if (tmp == NULL) {
#ifdef debug
               		    fprintf(stderr,
			    "GetContainerItems: couldn't calloc an DeviceData\n");
#endif
			    perror("calloc");
			    exit(2);
        	    }

		    if ( na > D_ARG ) {
			    tmp->DTP = strdup(dev[D_ARG]);
		    } else
			    tmp->DTP = strdup("");
		    tmp->holdModemFamily = tmp->modemFamily =strdup(dev[D_CALLER]);
		    tmp->holdPortSpeed = tmp->portSpeed =strdup(dev[D_CLASS]);
		    tmp->holdPortNumber = tmp->portNumber =strdup(port);

		    if ((op = new_object(name, tmp)) == (DmObjectPtr)OL_NO_ITEM) {
			continue;
		    }
                    if ( strcmp(dev[D_TYPE], ACU) == 0 )
			    op->fcp = acu_fcp;
		    else /* Direct */
			    op->fcp = dir_fcp;
		    AddObjectToContainer(op);
		}
        }
	if (fdevice != NULL) fclose(fdevice);
} /* GetContainerItems */

void
PutContainerItems(filename)
char* filename;
{
	FILE *fd;
	register DmObjectPtr op = df->cp->op;
	int	portNumber;
	int	i, nlines;
	DeviceData * tap;
	char* buf;
	char text [128];
	char type [16], line [16], dialer [64];
	char *lineptr[MAXLINE];

	nlines = 0;
	if ( df->cp->num_objs != 0 ) {
		for (op = df->cp->op; op; op = op->next) {
			if(nlines >= MAXLINE)
				break;
			tap = op->objectdata;
			if (strcmp(tap->modemFamily,"datakit") == 0) {
				strcpy(type, "DK");
				strcpy(dialer, "datakit");
			} else if (strcmp(tap->modemFamily,"direct") == 0) {
				strcpy(type, "Direct");
				strcpy(dialer, "direct");
			} else {
				strcpy(type, "ACU");
				strcpy(dialer, tap->modemFamily);
			}
			if (!strncmp(tap->portNumber, "com", 3)) {
				sscanf(tap->portNumber, "com%d", &portNumber);
				sprintf(line, "tty%.2d", portNumber-1);
			} else	sprintf(line, "%s", tap->portNumber);

			sprintf (text,
				"%s %s %s %s %s %s\n",
				type,
				line,
				"-",
				tap->portSpeed,
				dialer,
				tap->DTP);
			buf = malloc(strlen(text) +1 );
			strcpy (buf, text);
			lineptr[nlines++] = buf;
		}
	}
	if ((fd = fopen(filename, "w")) == NULL) {
		sprintf (text, GGT(string_fopenWrite), filename);
		XtVaSetValues (df->footer, XtNstring, text, (String)0);
		return;
	}
	fprintf (fd, "%s", lbuf);
	for (i=0; i < nlines; i++) {
		fprintf (fd, "%s", lineptr[i]);
		free(lineptr[i]);
	}
	XtVaSetValues (df->footer, XtNstring, GGT(string_saved), (String)0);
	fclose (fd);
	if (!exists) {
		chmod(filename, (mode_t) 0644);
		chown(filename, UUCPUID, UUCPGID);
		exists = 1;
	}
} /* PutContainerItems */

int
getargs(s, arps, count)
register char *s, *arps[];
register int count;
{
	char	buf[BUFSIZ];
        register int i;

	strcpy(buf, s);
        for (i = 0; /*TRUE*/ ;i++) {
                while (*s == ' ' || *s == '\t')
                        *s++ = '\0';
                if (*s == '\n')
                        *s = '\0';
                if (*s == '\0')
                        break;
                arps[i] = s++;
		if (i == count) {
			while (*s != '\0' && *s != '\n')
				s++;
			if(*s == '\n')
				*s = '\0';
			i++;
			break;
		}
                while (*s != '\0' && *s != ' '
                        && *s != '\t' && *s != '\n')
                                s++;
        }
	if (i < count)
		strcat(lbuf, buf);
        arps[i] = NULL;
        return(i);
}

void
FreeDeviceData(tap)
DeviceData * tap;
{
	free(tap->modemFamily);
	free(tap->holdModemFamily);
	free(tap->portSpeed);
	free(tap->holdPortSpeed);
	free(tap->portNumber);
	free(tap->holdPortNumber);
	if (tap->DTP)
	    free(tap->DTP);
	free(tap);
}

void
FreeFileClass(fcp)
DmFclassPtr fcp;
{
	free(fcp->glyph);
	free(fcp);
}

void
FreeObject(op)
DmObjectPtr op;
{
	free(op->name);
	FreeDeviceData(op->objectdata);
	free(op);
}

Boolean
getline(buf, len, fd)
char *buf;
int len;
FILE *fd;
{
	while (fgets(buf, len, fd) != (char *)NULL) {
		if (buf[0] == ' ' || buf[0] == '\t' ||  buf[0] == '\n'
		    || buf[0] == '\0' || buf[0] == '#') {
			strcat (lbuf, buf);
			continue;
		}
		return(True);
	}
	return(False);
} /* getline */
