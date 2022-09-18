/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)ypcmd:udpublickey.c	1.2.7.2"
#ident  "$Header: udpublickey.c 1.2 91/06/25 $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*
 * YP updater for public key map
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <sys/file.h>

# include <errno.h>
# include <sys/stat.h>
# include <sys/mac.h>

static int replace();

extern char *malloc();

main(argc, argv)
	int argc;
	char *argv[];
{
	unsigned op;
	char name[MAXNETNAMELEN + 1];
	char key[256];
	char data[256];
	char line[256];
	unsigned keylen;
	unsigned datalen;
	FILE *rf;
	FILE *wf;
	char *fname;
	char *tmpname;
	int err;


	if (argc !=  3) {
		exit(YPERR_YPERR);
	}
	fname = argv[1];
	tmpname = malloc(strlen(fname) + 4);
	if (tmpname == NULL) {
		exit(YPERR_YPERR);
	}
	sprintf(tmpname, "%s.tmp", fname);
	
	/*
	 * Get input
	 */
	if (! scanf("%s\n", name)) {
		exit(YPERR_YPERR);
	}
	if (! scanf("%u\n", &op)) {
		exit(YPERR_YPERR);
	}
	if (! scanf("%u\n", &keylen)) {
		exit(YPERR_YPERR);
	}
	if (! fread(key, keylen, 1, stdin)) {
		exit(YPERR_YPERR);
	}
	key[keylen] = 0;
	if (! scanf("%u\n", &datalen)) {
		exit(YPERR_YPERR);
	}
	if (! fread(data, datalen, 1, stdin)) {
		exit(YPERR_YPERR);
	}
	data[datalen] = 0;

	/*
	 * Check permission
	 */
	if (strcmp(name, key) != 0) {
		exit(YPERR_ACCESS);
	}
	if (strcmp(name, "nobody") == 0) {
		/*
		 * Can't change "nobody"s key.
		 */
		exit(YPERR_ACCESS);
	}

	/*
	 * Open files
	 */
	rf = fopen(fname, "r");
	if (rf == NULL) {
		exit(YPERR_YPERR);
	}
	wf = fopen(tmpname, "w");
	if (wf == NULL) {
		exit(YPERR_YPERR);
	}
	err = -1;
	while (fgets(line, sizeof(line), rf)) {
		if (err < 0 && match(line, name)) {
			switch (op) {
			case YPOP_INSERT:
				err = YPERR_KEY;
				break;
			case YPOP_STORE:
			case YPOP_CHANGE:
				fprintf(wf, "%s %s\n", key, data);
				err = 0;
				break;
			case YPOP_DELETE:
				/* do nothing */
				err = 0;
				break;
			}
		} else {
			fputs(line, wf);
		}
	}
	if (err < 0) {
		switch (op) {
		case YPOP_CHANGE:
		case YPOP_DELETE:
			err = YPERR_KEY;
			break;
		case YPOP_INSERT:
		case YPOP_STORE:
			err = 0;
			fprintf(wf, "%s %s\n", key, data);
			break;
		}
	}
	fclose(wf);
	fclose(rf);
	if (err == 0) {
		if (replace(tmpname, fname) < 0) {
			exit(YPERR_YPERR);	
		}
	} else {
		if (unlink(tmpname) < 0) {
			exit(YPERR_YPERR);
		}
	}
	if (fork() == 0) {
		close(0); close(1); close(2);
		open("/dev/null", O_RDWR, 0);
		dup(0); dup(0);
		execl("/sbin/sh", "sh", "-c", argv[2], NULL);
	}
	exit(err);
	/* NOTREACHED */
}


match(line, name)
	char *line;
	char *name;
{
	int len;

	len = strlen(name);
	return(strncmp(line, name, len) == 0 &&
		(line[len] == ' ' || line[len] == '\t'));
}





/*
 * Procedure:     replace
 *
 * Restrictions:
                 lvlfile(2): None!
                 unlink(2): None! 
                 chmod(2): None!
                 chown(2): None!
                 rename(2): None!
 * Procedure: replace - replace one file with another
 *
 * Notes: replace() resets the original
 *	  file's attributes after the rename if the original file
 *	  exists.
 *
 * Args:  tname - full path name of source file
 * 	  fname - full path name of target file
 */


int
replace(tname, fname)
char *tname;
char *fname;
{
	struct stat attr;	/* buffer for attributes */
	int attr_ret = 0;	/* found atttributes */
	level_t level;		/* MAC level identifier */
	
	/* get attributes if original file exists */ 
	if ( stat(fname, &attr) == 0 ) 
		attr_ret = 1;
	
	/*
	 * set to original mode & ownership
	 * set level only if attr.st_level has a valid value
	 */
	if ( attr_ret ) {
                if ( attr.st_level != 0 ) {
			level = attr.st_level;
			if ( lvlfile(tname, MAC_SET, &level) != 0) {
				(void)fprintf(stderr,
				"Could not set level on %s: %s\n", 
				fname, strerror(errno));
				(void) unlink(tname);
				return(-1);
			}
		}
		if (chmod(tname, attr.st_mode) != 0 ||
		    chown(tname, attr.st_uid, attr.st_gid) != 0) {
			(void)fprintf(stderr,
			"Could not set attributes on %s: %s\n", 
			fname, strerror(errno));
			(void) unlink(tname);
			return(-1);
		}
		
	}

	if ( rename(tname, fname) < 0) {
		(void) unlink(tname);
		(void)fprintf(stderr,
		"%s failed: %s\n", "rename()", strerror(errno));
		return(-1);
	}
	return(0);
}

