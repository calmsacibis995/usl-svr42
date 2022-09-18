/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/putdev.c	1.2.20.2"
#ident	"$Header: putdev.c 1.4 91/06/25 $"

/* LINTLIBRARY */

#include <stdio.h>	/* standard I/O definitions     */
#include <string.h>	/* string handling definitions  */
#include <ctype.h>	/* character types & macros     */
#include <errno.h>	/* error codes                  */
#include <stdlib.h>	/* storage alloc functions      */
#include <sys/types.h>	/* System data types            */
#include <sys/stat.h>	/* File status information      */
#include <sys/time.h>	/* Time definitions             */
#include <sys/mac.h>	/* MAC/SDH definitions          */
#include <fcntl.h>	/* File operations              */
#include <devmgmt.h>	/* global devmgmt definitions   */
#include <pwd.h>	/* passwd */
#include "devtab.h"	/* local devmgmt definitions    */

/*
 *  E X T E R N A L    R E F E R E N C E S
 *
 */
/*
 *
 * Functions defined in putdev.c:
 *
 *	ddb_create()		Creates new Devices Database files
 *	opentmpddb()		Opens/Creates a new temporary DDB file
 *	rmtmpddb()		Removes temporary DDB file
 *	adddevrec()		Add device & device attrs to DDB
 *	moddevrec()		Modify device attrs of device in DDB
 *	remdevrec()		Remove device from DDB
 *	rmdevattrs()		Remove device attrs of device from DDB
 *	appattrlist()		Append values to attrlist of device
 *	remattrlist()		Remove values from attrlist of device
 *	rmsecdevs()		Removes secdev attr from DDB_TAB & DDB_DSFMAP
 *	setddbinfo()		Set uid, gid, mac level on new DDB file
 */

/*
 *  L O C A L   D E F I N I T I O N S
 *
 * Static Functions defined in putdev.c:
 *
 *	mknewddb()		Rename temp file as new DDB file
 *      update_ddb()		Renames all temp DDB files as new DDB files
 *      getessentials()		Returns the essential security attributes for
 *				the specified alias
 *	valueinlist()		Returns SUCCESS or FAILURE if it finds the
 *				item being removed in the specified list.
 *
 */

static int 		update_ddb();
static int 		mknewddb();
static essentials 	*getessentials();
static int 		valueinlist();

/* Global variables */
char Cmdname[64] = "putdev";	/* Name of command (putdev/ddbconv) invoking
				 * libadm's function. To save a function call,
                                 * assume the command is putdev. */

int __tabversion__ = 0;

/*
 * Static Variables:
 *	DDB_files	Names of Device Database files
 *	ODDB_files	Names of old Device Database files
 *	Tmp_ddbfiles	Names of temporary DDB files
 *	Val_list[]	List of Attributes that take value-lists
 *			separated by commas.
 */
static char *DDB_files[] = { DDB_TAB, DDB_DSFMAP, DDB_SEC };
static char *ODDB_files[] = { ODDB_TAB, ODDB_DSFMAP, ODDB_SEC };
static char *Tmp_ddbfiles[MAXDDBFILES];
static char *Val_list[] = { DDB_CDEVLIST, DDB_BDEVLIST, DDB_USERS };



/*
 * Local defines:
 */
#define MINSECATTRS(range, state, mode)	((range != (char *)NULL) && \
					(state != (char *)NULL) && \
					(mode != (char *)NULL))

#define DDB_UMASK	0113	/* rw-rw-r-- */

#define SEC_UMASK	0117	/* rw-rw---- */

#define DDB_CMASK	0664	/* for device.tab and ddb_dsfmap */

#define SEC_CMASK	0660	/* for ddb_sec */

#define ETCDIR		"/etc"	/* if in 4.0, this directory is used to
				 * obtain the uid, gid that will be used
				 * when creating or updating the DDB file 
				 */
							

#define SECDIR		"/etc/security/ddb"	/* DDB directory. It has 
					       	 * same uid, gid, and MAC level
					 	 * as the DDB files. Used in 
						 * the cases where device.tab	
						 * doesn't exit */

#define MAXVAL_LIST	3	/* no. of attrs that take value-list */

#define QUIT_PUTDEV(devrec) {          \
		free_devrec(&devrec);\
		unlock_ddb();       \
		return(FAILURE);    \
		}

#define LOGNAMEMAX	32

#define CDEVLIST	0
#define BDEVLIST	1
#define USERS		2

/*
 *  int ddb_create()
 *
 *	This function creates new Device Database files.
 *
 *  Arguments:
 *
 *  Returns:  int
 *	SUCCESS if successful, FAILURE otherwise
 *
 */

int
ddb_create()
{
	FILE	*f_tab, *f_dsf, *f_sec;	/* file descriptors */
	mode_t	old_umask;
	struct 	stat  statbuf;
	int	ret = 0;


	/* DDB_TAB and DDB_DSFMAP DAC permission = rw-rw-r-- */
	old_umask = umask((mode_t)DDB_UMASK);

	/* 
	 * Get the uid, gid, and mac level of the DDB_TAB file, if it exists.
	 * The uid, gid, and mac level will be assigned
	 * to the newly created files. If this is the first time
	 * the ddb files are created and /etc/device.tab doesn't
	 * exit, use directory /etc/security/ddb's uid/gid/mac level
	 * values which are the same as the DDB files. 
	 */
	if ((stat(DDB_TAB, &statbuf)) == -1) {

		if (__tabversion__ == __4ES__) 
			ret = stat(SECDIR, &statbuf);
		else 
			ret = stat(ETCDIR, &statbuf);

		if (ret == -1 ) {
			(void)umask(old_umask);
			ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			return(FAILURE);
		} 
	}
	/* create new magic number */
	cr_magicno();

	if (__tabversion__ == __4dot0__) {

		if ((f_tab = fopen(DDB_TAB,"w")) == (FILE *)NULL) {
			(void)umask(old_umask);
			ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			return(FAILURE);
		} 
		fclose(f_tab);
		if (setddbinfo(DDB_TAB, statbuf) == FAILURE) {
			(void)umask(old_umask);
			ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			return(FAILURE);
		}
		return(SUCCESS);

	}

	/* create DDB_TAB & DDB_DSFMAP files */
	if (((f_tab=fopen(DDB_TAB, "w")) != (FILE *)NULL) &&
		((f_dsf=fopen(DDB_DSFMAP, "w")) != (FILE *)NULL)) {
	    /* set magic no */
	    setmagicno(f_tab);
	    setmagicno(f_dsf);

	    if (_mac_installed()) {

		/* DDB_SEC DAC permissions = rw-rw---- */
		(void) umask((mode_t)SEC_UMASK);
		if ((f_sec=fopen(DDB_SEC, "w")) != (FILE *)NULL) {
		    /* set magic number */
		    setmagicno(f_sec);
		    fclose(f_sec);

		    /* Set uid, gid, and mac level to the original DDB_TAB */
		    if ((setddbinfo(DDB_SEC,statbuf)) == FAILURE) {
		    	(void)umask(old_umask);
		    	ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);

		    	/* close other opened files before returning */
		    	fclose(f_tab);
		    	fclose(f_dsf);
		    	return(FAILURE);
		    }
		} else {
		    (void)umask(old_umask);
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    /* close other opened files before returning */
		    fclose(f_tab);
		    fclose(f_dsf);
		    return(FAILURE);
		}
	    }
	    fclose(f_tab);
	    fclose(f_dsf);
	    (void)umask(old_umask);
	} else {
	    (void)umask(old_umask);
	    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
	    return(FAILURE);
	}

	/* Set uid, gid, and mac level of new DDB_TAB 
	 * to the values contained in statbuf.
	 */
	if ((setddbinfo(DDB_TAB,statbuf)) == FAILURE) {
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		return(FAILURE);
	}

	/* Set uid, gid, and mac level of new DDB_DSFMAP
	 * to the values contained in statbuf.
	 */
	if ((setddbinfo(DDB_DSFMAP,statbuf)) == FAILURE) {
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		return(FAILURE);
	}
	return(SUCCESS);
}
/*
 * FILE *opentmpddb(ddbfile, tmpname)
 *	char    *ddbfile;
 *	char   **tmpname
 *
 *	Generates a temporary DDB file, <tmpname>, from the specified 
 *	DDB filename, <ddbfile>. It opens file <tmpname> for writing.
 *	It returns a pointer to the malloc()ed space
 *	containing the temp DDB filename at the place referenced
 *	by <pname>.
 *
 *  Arguments:
 *	ddbfile		DDB filename used to create temp file <tmpname>.
 *	tmpname		Address of pointer to the char string that 
 *			contains the name of temporary file created.
 *
 *  Returns:  FILE *
 *	A pointer to the opened stream or (FILE *) NULL if an error occurred.
 *	If an error occurred, msgbuf will be setup with error code.
 */

FILE *
opentmpddb(char *ddbfile, char **tmpname)
{
	char	tmpddb[MAXDDBPATH];	/* buffer for temp file's name    */
	char	pid[MAXIDSZ]; 		/* curr process id in ASCII       */
	FILE	*fd;			/* Opened file descriptor         */
	int	fileno;			/* index into DDB_files array     */
	mode_t	oldumask; 		/* Old value for umask            */

	fd = (FILE *) NULL;

	/* get index into array of valid DDB files */
	for (fileno=0 ; fileno<MAXDDBFILES ; fileno++) {
	    if (strcmp(DDB_files[fileno], ddbfile) == 0) break;
	}
	/* create temp file name using current process id */
	sprintf(tmpddb, "%s%6.6ld", ddbfile, getpid());

	/* save old umask of DDB file */
	if (strcmp(ddbfile,DDB_SEC) == 0)
		oldumask = umask((mode_t) SEC_UMASK);
	else
		oldumask = umask((mode_t) DDB_UMASK);

	/* open temp DDB file for write */
	if ( (fd = fopen(tmpddb, "w")) == (FILE *)NULL) {
		/* error, could not create temp file */
		Tmp_ddbfiles[fileno] = (char *)NULL;
		*tmpname = (char *)NULL;
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		return((FILE *)NULL);
	}

	if (Tmp_ddbfiles[fileno]=(char *)malloc(strlen(tmpddb) + 1)) {

		/* copy temp filename to static buffer */
		strcpy(Tmp_ddbfiles[fileno], tmpddb);

		/* return pointer to temp filename */
		*tmpname = Tmp_ddbfiles[fileno];
	 } else {
		/* ran out of memory */
		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		return((FILE *)NULL);
	}

	/* Finished.  Return what we've got */
	return(fd);
}

/*
 *  int rmtmpddb(tmpname)
 *	char	*tmpname;
 *
 *	Remove the temporary DDB file, <tmpname>, and free memory allocated
 *	to save the name of the temporary file.
 *
 *  Arguments:
 *	tmpname		Name of the temporary file to be removed
 *
 *  Returns:  int
 *	SUCCESS if successful, FAILURE otherwise
 *
 */

int
rmtmpddb(char *tmpname)
{
	int	fileno;			/* index into DDB_files[]       */

	/* get index into array of valid temp DDB files */
	for (fileno=0 ; fileno<MAXDDBFILES ; fileno++) {
	    if (strcmp(Tmp_ddbfiles[fileno], tmpname) == 0) break;
	}

	/* remove the temp DDB file */
	if (remove(tmpname) == 0) {
	    /* free temp filename space */
	    free(tmpname);
	    Tmp_ddbfiles[fileno] = (char *)NULL;
	    return(SUCCESS);
	}

	/* error, could not remove file   */
	return(FAILURE);
}

/*
 *  static int ddb_copy(source, target)
 *	char	*source;
 *	char	*target;
 *
 *	Copies <source> DDB file to the <target> DDB file.
 *
 *  Arguments:
 *	<source>	Source DDB filename.
 *	<target>	Target DDB filename.	
 *  Returns:  int
 *	SUCCESS if successful, FAILURE otherwise
 */
static int
ddb_copy(source, target)
char	*source;
char	*target;
{
	int	fs, ft, n;
	char	buf[1024];

	/* open source DDB file for read */
	if ((fs=open(source, O_RDONLY)) < 0) {
	    return(FAILURE);
	}

	/* open target DDB file for write */
	if (strcmp(target, ODDB_SEC) == 0) {
		if ((ft=open(target, O_WRONLY|O_CREAT, SEC_CMASK)) < 0) {
	    		close(ft);
	    		return(FAILURE);
		}
	}
	else if ((ft=open(target, O_WRONLY|O_CREAT, DDB_CMASK)) < 0) {
	    	close(ft);
	    	return(FAILURE);
	}

	/* copy from source to target file in 1 K byte chunks */
	for (;;) {
	    if ((n=read(fs, buf, 1024))>0) {
		if(write(ft, buf, n) != n) {
		    close(fs);
		    close(ft);
		    return(FAILURE);
		}
	    } else if (n == 0) {
		/* copy done */
		break;
	    } else {
		/* read failed */
		close(fs);
		close(ft);
		return(FAILURE);
	    }
	}

	/* close source & target files */
	close(fs);
	close(ft);

	return(SUCCESS);
}

/*
 *  static int mknewddb(fileno)
 *	int	fileno;
 *
 *	Rename the temporary DDB file, Tmp_ddbfiles[fileno], as the new 
 *	DDB file with the corresponding filename in DDB_files[fileno].
 *	The old DDB file is saved in the file ODDB_files[fileno].
 *
 *	If there is no temporary file existing (Tmp_ddbfiles[fileno]=NULL),
 *	then the magic number on the current DDB file is updated.
 *
 *  Arguments:
 *	fileno		index into the Tmp_ddbfiles[], DDB_files[], and
 *			ODDB_files[] arrays.
 *
 *  Returns:  int
 *	SUCCESS if successful, FAILURE otherwise
 */

static int
mknewddb(fileno)
int	fileno;			/* index into DDB_files[]       */
{
	FILE	*fp;
	struct	stat statbuf;
	int ret = 0;

	/* Get information from the original file /etc/device.tab file.
	 * If that file is not there, then try using directory 
	 * /etc/security/ddb which has the same ownership and label.
	 * If neither exists, exit. Otherwise, use this information
	 * to set the temporary file before renaming it
	 */
	if ((stat(DDB_TAB, &statbuf)) == -1) {

		if (__tabversion__ == __4ES__)
			ret = stat(SECDIR, &statbuf);
		else 
			ret = stat(ETCDIR, &statbuf);

		if (ret == -1) {
			ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			return(FAILURE);
		}
	}

	/* check if temporary file exists? */
	if (Tmp_ddbfiles[fileno]) {

	    /* Set uid, gid, and mac level of new ddb files */
	    if ((setddbinfo(Tmp_ddbfiles[fileno],statbuf)) == FAILURE) {
		    
		    /* Couldn't changed uid/gid/level,
		     * set error buffer and return FAILURE */
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    return(FAILURE);
	    }

	    /* Yes, then save current DDB file as old DDB file *
	     * and rename temp. DDB file as current DDB file.  */ 
	    if (rename(DDB_files[fileno], ODDB_files[fileno]) == 0) {

		/* rename the temp file to corresponding DDB file */
		if (rename(Tmp_ddbfiles[fileno], DDB_files[fileno]) == 0) {
		    /* free temp filename space */
		    free(Tmp_ddbfiles[fileno]);
		    Tmp_ddbfiles[fileno] = (char *)NULL;

		} else {
		    /* error, could not rename temp. file */
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    return(FAILURE);
		}

	    } else {
		/* severe error, could not move DDB file to old DDB file*/
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		return(FAILURE);
	    }

	} else {
	    /* Temp. DDB file does NOT exist. Save the current DDB *
	     * file in old DDB file, and update only the Magic-no  *
	     * in the current DDB file.                            */

	    if (ddb_copy(DDB_files[fileno], ODDB_files[fileno])==SUCCESS) {
		/* set magic-no in current DDB file */
		if (fp=fopen(DDB_files[fileno],"r+")) {
			setmagicno(fp);
			fclose(fp);
		} else {
			ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			return(FAILURE);
		}
	    } else {
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		return(FAILURE);
	    }
	}
	/* Finished.  Return success indicator */
	return(SUCCESS);
}

/*
 *  static int update_ddb()
 *
 *	Renames all of the temporary DDB file, in Tmp_ddbfiles[] to
 *	the actual DDB filenames as in DDB_files[] array.
 *
 *  Arguments:
 *
 *  Returns:  int
 *	SUCCESS if successful, FAILURE otherwise
 *
 *  Notes:
 *	- Uses the syscall rename() to accomplish this.
 */

static int
update_ddb()
{
	int i, j;

	if (__tabversion__ == __4dot0__) {

		/* If in 4.0, only update the DDB_TAB file */
		if (mknewddb(0) == FAILURE) {
			rmtmpddb(Tmp_ddbfiles[0]);
			return(FAILURE);
		}
		return(SUCCESS);

	}

	/* Update DDB_TAB and DDB_DSFMAP */
	for (i=0 ; i<(MAXDDBFILES-1) ; i++) {

	    if (mknewddb(i) == FAILURE) {

		/* If there was a failure, remove all 
		 * temporary files that were created */
		for (j=0 ; j<MAXDDBFILES ; j++) {
			if (Tmp_ddbfiles[j])
				rmtmpddb(Tmp_ddbfiles[j]);
		}
		return(FAILURE);
	    }
	}	

	/* Update DDB_SEC file ONLY if enhanced secpkg is installed */
	if (_mac_installed()) {

	    if (mknewddb(2) == FAILURE) {

			/* remove ddb's sec temporary file. If we get 
		 	 * to this point, the other temporary files should
		 	 * have been alreadyremoved by mknewddb */

			rmtmpddb(Tmp_ddbfiles[2]);
			return(FAILURE);
	    }
	}
	return(SUCCESS);
}

/*
 *  int adddevrec(alias, attrval, ddblock)
 *	char   *alias
 *	char  **attrval
 *	int 	ddblock
 *
 *	This function adds a new device and its device attributes to the
 *	Device Database (DDB). The device must have an unique alias name
 *	<alias>, and a set of security or non-security attributes defined
 *	in the list referenced by <attrval>.
 *
 *  Arguments:
 *	alias		The alias of the device whose description is being
 *			added to the DDB.
 *	attrval		The pointer to the first item of a list of attributes
 *			defining the device whose description is being added.
 *			(This value may be (char **) NULL).
 *	ddblock		tells the function if the DDB is already locked.
 *			The flag is implicitly used to determine which command
 *			called the function
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 */

int
adddevrec(char *alias, char **attrval, int ddblock)
{
	dev_record		devrec;		/* device record        */
	char			sdev[DDB_MAXALIAS]; /* secure dev alias */
	int			entype;		/* entry type of attr   */
	int			atype;		/* alias type           */
	int			tabinfo, mapcnt;
	int			warning = 0;	/* 0 no warning, 1 warning */
	essentials		*sesa;
	char			*aptr;


	if (__tabversion__ == 0) {

		__tabversion__ = gettabversion();

		if (__tabversion__ == FAILURE) {
	    		ddb_errmsg(SEV_ERROR,EX_ACCESS,E_ACCESS);
			return(FAILURE);
		}
	}

	/* Validate the device alias */
	if (!valid_alias(alias)) {
	    ddb_errmsg (SEV_ERROR, EX_ERROR, E_DINVAL, alias);
	    return(FAILURE);
	}

	/* if ddblock == TRUE, then ddb has been locked.
	 * Otherwise, proceed to lock ddb.
	 */
	
	if (ddblock != TRUE) {

		/* If ddb is not locked, the invoker must be ddbconv */
		strcpy(Cmdname,DDBCONV);

		/* Lock the Device Database */
		if (lock_ddb() != SUCCESS) {
	    		/* unable to lock Device Database */
	    		switch (ddb_errget()) {
			case (EAGAIN):
	    			ddb_errmsg(SEV_ERROR,EX_ERROR,E_DDBUSE);
	    			break;
			default:
	    			ddb_errmsg(SEV_ERROR,EX_ACCESS,E_ACCESS);
	    			break;
	    		}
	    		return(FAILURE);
		}
	} 

	/* Make sure that the alias isn't already in DDB */
	if ((tabinfo=getaliasmap(alias, &atype, &mapcnt, sdev)) < 0) {
		/* DDB files could not be accessed */
		ddb_errmsg (SEV_ERROR, EX_ACCESS, E_ACCESS);
		unlock_ddb();
		return(FAILURE);
	} else if (tabinfo) {
    		/* The alias is already defined in DDB_TAB */
		ddb_errmsg(SEV_ERROR, EX_ERROR, E_AEXIST, alias);
		unlock_ddb();
		return (FAILURE);
	}

	/*
	 * Extract attr names and values from <attrval> attr list
	 *  - determine attr type; which devrec field it belongs in
	 * 	- validate each attr value
	 *  - initialize corresponding entry field
	 */
	if ((entype=make_devrec(alias, attrval, DEV_ADD, &devrec))<=0) {
		unlock_ddb();
		return(FAILURE);
	}


	/*
	 ** Validate the following:
	 *     if (enhanced sec. installed) {
	 *         if (secdev defined and not equal to alias) {
	 *             * only oam and dsf attrs should be defined *
	 *             if (sec attrs also defined) ddb_errmsg();
         *             if (alias already used as secdev) ddb_errmsg();
	 *             if (secdev does not point to secure dev) ddb_errmsg();
	 *             if (secdev not currently in DDB) ddb_errmsg(WARNING);
	 *         } else { 
	 *             * secdev not defined or secdev = alias *
	 *		* default behavior: set secdev = alias *
         *             if (alias already used as secdev and not defining
	 * 		    the essential security attributes) ddb_errmsg();
         *             if (specifying sec attributes but not the essential
	 *		    sec attrs not defined) ddb_errmsg();
	 *         }
         *     } else {
	 *         * enhanced sec. NOT installed  *
	 *         if (sec attrs defined) OR (secdev defined) ddb_errmsg();
	 *     }
         *
	 */

	/* Enhanced security package installed ? */ 
	if (!_mac_installed()) {
		/* enhanced security package not installed */
		if ((entype & TYPE_SEC)||(devrec.tab->secdev)) {
		    /* sec attrs defined or secdev defined */
		    ddb_errmsg(SEV_ERROR, EX_NOPKG, E_NOPKG, Cmdname);
		    QUIT_PUTDEV(devrec);
		}
	} else {
		/* if secdev defined and not equal to alias */
		if((devrec.tab->secdev)&&(strcmp(alias,devrec.tab->secdev)!=0)){

		    if (entype & TYPE_SEC) {
			/* if sec attrs are also defined for alias */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOTSEC, alias);
			QUIT_PUTDEV(devrec);
		    }

		    if (mapcnt > 0) {
			/* alias already being mapped to by other devices *
			 * alias must define sec attrs -- a secure device */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_SECENT, alias);
			QUIT_PUTDEV(devrec);
		    }

		    /* Get essential security attrs from DDB for specified
		     * secdev. If secdev is not defined in DDB, the values
		     * will be NULL 
		     */
		    sesa = getessentials(devrec.tab->secdev);

		    if(aptr=devattr(devrec.tab->secdev, DDB_SECDEV)) {
			
			if (!MINSECATTRS(sesa->range,sesa->state,sesa->mode)){
			    /* secdev points to device alias that does *
			     * not define essential sec attrs          */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSSEC, devrec.tab->secdev);
			    QUIT_PUTDEV(devrec);
			}

		    } else {

			/* secdev not yet defined in DDB */
			ddb_errmsg(SEV_WARN,EX_ERROR,E_ANOTDEF,devrec.tab->secdev);
			/* set warning flag */
			warning = 1;

			/* display WARNING message */
			err_report(Cmdname, ACT_CONT);

		    }

		} else {
		    /* secdev not defined OR secdev equals alias */
		    if (devrec.tab->secdev == NULL) {
	
				/* secdev=NULL, set secdev=alias by default */
				if (aptr=(char *)malloc(strlen(alias)+1)) {
			    		strcpy(aptr,alias);
					/* oam attrs by default */
			    		entype |= TYPE_TAB;	
			    		devrec.tab->secdev = aptr;
				} else {
			    		ddb_errmsg(SEV_ERROR,EX_INTPRB,E_NOMEM);
			    		QUIT_PUTDEV(devrec);
				}

		    }

		    /* if other logical aliases already map to this alias */
		    if (mapcnt > 0) {
			/* specified alias must define essential security
			 * attributes in the command line */
			if(!MINSECATTRS(devrec.sec->range,devrec.sec->state,
							devrec.sec->mode)) {
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_SECENT,alias);
			    QUIT_PUTDEV(devrec);
			}
		    } 

		    /* If specifying security attributes, make sure the
		     * essential security attributes are also specified
		     */
		    if (entype & TYPE_SEC) {

			/* minimum security attributes defined ? */
			if (!MINSECATTRS(devrec.sec->range,devrec.sec->state,
							devrec.sec->mode)) {
			    ddb_errmsg(SEV_ERROR,EX_ERROR,E_ESSSEC, alias);
			    QUIT_PUTDEV(devrec);
			}
		    }
		}
	}

	/*
	 * Add entries to appropriate DDB files
	 */

	if (__tabversion__ == __4ES__) {
		/* if not in 4.0, create new magic number */
		cr_magicno();
	}

	if (entype & TYPE_SEC) {
		/* initialize alias field of sec_entry */
		if (aptr=(char *)malloc(strlen(alias)+1)) {
		    strcpy(aptr,alias);
		    devrec.sec->alias = aptr;
		} else {
		    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		    QUIT_PUTDEV(devrec);
		}
		/* add sec_entry to temp DDB_SEC file */
		if (put_secent(devrec.sec) == FAILURE)
		    QUIT_PUTDEV(devrec);
	}

	if (__tabversion__ == __4ES__) {

		/* If in 4.0, skip this code. If cdevice or bdevice are
		 * specified in the command line, <entype> will be TYPE_DSF, 
		 * but if in 4.0 the call to put_dsfent must be skipped.
		 * The values for c/bdevice are also set on devrec.tab
		 * and they will be handled by the call to put_tabent().
		 */

	    	if (entype & TYPE_DSF) {

			/* add dsf_entry to temp DDB_DSFMAP file */
			if(put_dsfent(devrec.dsf,alias,devrec.tab->secdev) == FAILURE) {
		    		/* remove temp DDB_SEC file, if file was created */
		    		rmtmpddb(Tmp_ddbfiles[2]);
		    		QUIT_PUTDEV(devrec);
			}

	    	}

	}

	if (entype & TYPE_TAB) {

		/* add tab_entry to temp DDB_TAB file */
		if(put_tabent(devrec.tab) == FAILURE) {
		    /* remove temporary DDB_SEC and DDB_DSFMAP
		     * files, if they were created */
		    rmtmpddb(Tmp_ddbfiles[2]);
		    rmtmpddb(Tmp_ddbfiles[1]);
		    QUIT_PUTDEV(devrec);
		}

	}

	/* rename the temp files to DDB filenames */
	if(update_ddb()<0) {
	 	QUIT_PUTDEV(devrec);
	}

	/* free memory allocated for attributes in devrec */
	free_devrec(&devrec);

	/* Unlock the Device Database */
	unlock_ddb();

	/* return warning to invoking command 
	 * so that appropriate exit status is returned
	 */
	if (warning)
		return(WARNING);

	/* Fini */
	return(SUCCESS);
}

/*
 *  int moddevrec(device, attrval, ddblock)
 *	char	*device
 *	char	**attrval
 *	int 	ddblock
 *
 *	This function modifies the attributes of an existing <device>
 *	in the Device Database (DDB). The <device> could be an alias
 *	name or an absolute pathname to a device special file.
 *	If <device> is a pathname of dsf, then the alias to which the
 *	dsf maps is used for modiification. 
 *
 *  Arguments:
 *	device		an alias name or absolute pathname to dsf.
 *	attrval		The pointer to the first item of a list of attributes
 *			defining the device whose description is being added.
 *			(This value may be (char **) NULL).
 *	ddblock		tells the function if the DDB is already locked.
 *			The flag is implicitly used to determine which command
 *			called the function
 *
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 *	NOATTR		if an attribute being removed was not defined for
 *			the device (This is for 4.0 compatibility)
 */

int
moddevrec(char *device, char **attrval, int ddblock)
{
	dev_record	devrec;		/* dev_record structure */
	tab_entry	*tab;
	char		alias[DDB_MAXALIAS],  /* alias name        */
			salias[DDB_MAXALIAS], /* secure dev alias  */
			sdev[DDB_MAXALIAS]; /* secure dev alias    */
	int		entype;		/* entry type of attr      */
	int		dtype;		/* dsf type                */
	int		atype;		/* alias type              */
	int		modcmd;		/* type of modify option   */
	int		tabinfo, mapcnt;
	int		ret = SUCCESS; 
	int		warning = 0;  	/* 0 = no warning, 1 = warning*/

	int		secdevissecure = 0; /* set if the specified secdev
					     * is defined in DDB and has
					     * sec attrs defined.
					     */
	essentials	*sesa, *aesa;	   /* pointers to specified secdev
					    * and specified alias essential
					    * security attributes, if defined
					    */
	char		*aptr, *ddbrange;

	/* Make sure that the variable is initialized*/
	if (__tabversion__ == 0) {

		__tabversion__ = gettabversion();

		if (__tabversion__ == FAILURE) {
	    		ddb_errmsg(SEV_ERROR,EX_ACCESS,E_ACCESS);
			return(FAILURE);
		}
	}


	/* Validate the device alias */
	if (!(valid_alias(device) || valid_path(device))) {
	    ddb_errmsg (SEV_ERROR, EX_ERROR, E_DINVAL, device);
	    return(FAILURE);
	}
	modcmd = DEV_MOD;	/* modify all attrs except <secdev> */

	/* If the caller hasn't lock the Device Database, lock it */
	if (ddblock != TRUE) {
		strcpy(Cmdname, DDBCONV);
		if (lock_ddb() != SUCCESS) {
	    		/* unable to lock Device Database */
	    		switch (ddb_errget()) {
			case (EAGAIN):
		    		ddb_errmsg(SEV_ERROR, EX_ERROR, E_DDBUSE);
		    		break;
			default:
		    		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    		break;
	    		}
	    		return(FAILURE);
		}
	} 

	if (valid_path(device)) {

		/* get the alias name <device> maps to */
		if (__tabversion__ == __4ES__) {
			if (getdsfmap(device, &dtype, alias, salias) < 0) {
		    		unlock_ddb();
		    		ddb_errmsg(SEV_ERROR,EX_ENODEV,E_NODEV,device);
		    		return(FAILURE);
			}
		} else {
			/* If argument is an absolute pathname, and in 4.0,
			 * call getalias() to search in the DDB_TAB file
			 * for the first alias whose cdevice, bdevice or
			 * pathname attribute matches <device>. 
			 * This is no good, but that's how it was done in 4.0.
			 */
			tab = getalias(device,&dtype);
			if (tab->alias == (char *)NULL) {
		    		unlock_ddb();
		    		ddb_errmsg(SEV_ERROR,EX_ENODEV,E_NODEV,device);
		    		return(FAILURE);
			}
			strcpy(alias,tab->alias);
		}

	} else {
		/* <device> should be an alias name */
		strcpy(alias, device);
	}

	/* Make sure that the device is already in DDB_TAB file */
	if ((tabinfo=getaliasmap(alias, &atype, &mapcnt,salias)) < 0) {
		/* DDB_TAB file could not be accessed */
		unlock_ddb();
		ddb_errmsg (SEV_ERROR, EX_ACCESS, E_ACCESS);
		return(FAILURE);
	} else if (tabinfo == 0) {
		/* The alias does not exist in DDB */
		unlock_ddb();
		ddb_errmsg(SEV_ERROR, EX_ENODEV, E_NODEV, alias);
		return (FAILURE);
	}

	/*
	 * Extract attr names and values from <attrval> attr list
	 *  - determine attr type; which devrec field it belongs in
	 * 	- validate each attr value
	 *  - initialize corresponding entry field
	 */
	if ((entype=make_devrec(alias, attrval, DEV_MOD, &devrec))<=0) {
		unlock_ddb();
		return(FAILURE);
	}
	/*
	 ** Validate the following:
	 *	if (enhanced sec. is not installed) {
	 *         if (sec attrs specified) OR (secdev specified) ddb_errmsg();
	 *	}
	 *
	 *	if (modifying secdev) {
	 *       if (secdev specified is not equal to alias) {
	 *         * only oam and dsf attrs should be defined *
	 *         if (sec attrs also defined) ddb_errmsg();
	 *         if (alias mapped to by other aliases) ddb_errmsg();
	 *	   if (alias has ess sec attrs already defined) ddb_errmsg();
	 *         if (new secdev does not point to secure dev) ddb_errmsg();
	 *         if (new secdev not currently in DDB) ddb_errmsg(WARNING);
	 *       } else {
	 *	   if (secdev specified equal to alias' secdev) {
	 *	     if (modifying a security attribute) {
	 *	       if (essentials are not defined in DDB or being added)
	 *	  		ddb_errmsg()
	 *	      }
	 *         }
	 *	 }
         *	} else {
	 *	     if (alias = secdev) {
	 *	        if (modifying sec attrs)
	 *		   if (ess sec not defined in DDB and not being added)
	 *			ddb_errmsg();
	 *	     }
	 * 	     if (alias's type is not secure and modifying sec attrs)
	 *	 	 ddb_errmsg()
	 *	}
	 */

	/* Enhanced security package installed ? */ 
	if (!_mac_installed()) {
		/* enhanced security package not installed */
		if ((entype & TYPE_SEC)||(devrec.tab->secdev)) {
		    /* sec attrs defined or secdev defined */
		    ddb_errmsg(SEV_ERROR, EX_NOPKG, E_NOPKG,Cmdname);
		    QUIT_PUTDEV(devrec);
		}
	} else {

	/* secdev being modified for device */
	if (devrec.tab->secdev) {

		/* Get the essential security attributes from the DDB for the
		 * specified secdev. If they  are not defined,
		 * the values will be set to (char *)NULL.
		 */
		sesa = getessentials(devrec.tab->secdev);
		secdevissecure = MINSECATTRS(sesa->range,sesa->state,
				sesa->mode);

	        /* if secdev defined is not equal to alias */
		if (strcmp(alias, devrec.tab->secdev)!=0) {

		    /* secdev not equal to alias */
		    if (entype & TYPE_SEC) {
			/* cannot define secdev!=alias + security attrs */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOTSEC, alias);
			QUIT_PUTDEV(devrec);
		    }

		    if (mapcnt > 0) {
			/* alias being modified is mapped to by other aliases*
			 * Therefore, must define secdev=alias               */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOTSEC, alias);
			QUIT_PUTDEV(devrec);
		    }

		    /* check if alias has ess sec attrs already defined */
		    if(ddbrange = devattr(alias, DDB_RANGE)) {
			/* cannot define security attrs and secdev!=alias */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOTSEC, alias);
			QUIT_PUTDEV(devrec);
		    }

		    /* is the specified secdev defined in the DDB? */
		    if (devattr(devrec.tab->secdev,DDB_SECDEV)!=(char *)NULL) {

		    	/* check if secdev defines essential security attrs */
		    	if(!secdevissecure) {
				ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSSEC,
					devrec.tab->secdev);
			    QUIT_PUTDEV(devrec);
			}
		    
		    } else {

			/* secdev not yet defined in DDB        */
			ddb_errmsg(SEV_WARN,EX_ERROR,E_ANOTDEF,
				devrec.tab->secdev);

			warning = 1;

			/* display WARNING message */
			err_report(Cmdname, ACT_CONT);
		    }
		} 

		/* if new secdev is different from secdev in DDB */
		if ((strcmp(salias, devrec.tab->secdev))!=0) {
			modcmd = DEV_MODSEC;
		    	/* copy new secdev to secure device alias */
		    	strcpy(salias, devrec.tab->secdev);

		} else {

			/* The specified secdev is the same as secdev in DDB */

			/* Were any of the specified attributes
			 * a security attribute?
			 */
			if (entype & TYPE_SEC)  {

				/* If neither the specified secdev
				 * has the essential security attrs defined
				 * in the DDB, nor are the essential
				 * security attrs specified in the command
				 * line, issue error message */
				
				if (!secdevissecure) {

					/* Check if any of the ess. sec. attrs.
					 * is defined in the command line.
					 */
					if(devrec.sec->range != (char *)NULL || 
					   devrec.sec->state != (char *)NULL ||
				   	   devrec.sec->mode != (char *)NULL) {

						/* Make sure that if the any of
						 * the ess sec attrs is defined 
						 * in the command line, then all
						 * are specified together. 
						 * Otherwise issue errror
						 */
						if (!MINSECATTRS(devrec.sec->mode,devrec.sec->state, devrec.sec->range)) {
			    				ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSTOG, devrec.tab->secdev);
			    				QUIT_PUTDEV(devrec);
						}

					} else  {
			    			ddb_errmsg(SEV_ERROR, EX_ERROR,
							E_ESSSEC,
							devrec.tab->secdev);
			    			QUIT_PUTDEV(devrec);
					}
				}
			}
	      }

	} else {

		aesa = getessentials(alias);

		/* secdev not being modified */
		if (atype == DEV_SECDEV) {

		    /* alias being modified is a secure alias */
		    if (entype & TYPE_SEC) {

			/* does alias has ess sec attrs defined in the DDB? */
			if (!MINSECATTRS(aesa->range,aesa->state,aesa->mode)) {

				/* Check if any of the essential security
				 * attributes is defined in the command line.
				 */
				if (devrec.sec->range != (char *)NULL ||
				    devrec.sec->state != (char *)NULL || 
				    devrec.sec->mode != (char *)NULL) {

				    if (!MINSECATTRS(devrec.sec->range,
					 devrec.sec->state,
					 devrec.sec->mode)) {

					/* some but not all the essential
					 * sec attrs are defined in cmd line*/
				        ddb_errmsg(SEV_ERROR,
					   EX_ERROR, E_ESSTOG,alias);
				        QUIT_PUTDEV(devrec);

				    }
				} else {

					/* trying to modify a security attr
				 	 * but the alias doesn't have the
				 	 * essentials defined in DDB or in
					 * the command line */
			    		ddb_errmsg(SEV_ERROR, EX_ERROR,
			  	   	E_ESSSEC, alias);
			        	QUIT_PUTDEV(devrec);

				}
			}
		    }
		}

		if (atype == DEV_ALIAS) {
		    /* alias being modified is a logical alias */
		    if (entype & TYPE_SEC) {
			/* cannot define sec. attrs for logical alias */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOTSEC, alias);
			QUIT_PUTDEV(devrec);
		    }
		}
	 }
	}

	/*
	 * modify entries in appropriate DDB files
	 */
	/* create new magic number. */
	cr_magicno();

	if (entype & TYPE_SEC) {
		if (aptr=(char *)malloc(strlen(alias)+1)) {
	            strcpy(aptr,alias);
	            devrec.sec->alias = aptr;
		} else {
		    unlock_ddb();
		    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		    return (FAILURE);
	        }
		/* modify sec_entry in temp DDB_SEC file */
		if (mod_secent(devrec.sec, DEV_MOD) < 0) {
		    QUIT_PUTDEV(devrec);
		}
	}

	if (__tabversion__ == __4ES__) {

		/* If in 4.0, skip this code. If cdevice or bdevice are
		 * specified in the command line, <entype> will be TYPE_DSF, 
		 * but if in 4.0 the call to put_dsfent must be skipped.
		 * The values for c/bdevice are also set on devrec.tab
		 * and they will be handled by the call to put_tabent().
		 */

		/* check if dsf attrs OR secdev being modified */
		if ((entype&TYPE_DSF)||(modcmd==DEV_MODSEC)) {
	
			/* modify dsf_entry in temp DDB_DSFMAP file */
			if (mod_dsfent(devrec.dsf,alias,salias,modcmd) < 0) { 
				/* remove temp DDB_SEC file,
				 * if it was created */
				rmtmpddb(Tmp_ddbfiles[2]);
				QUIT_PUTDEV(devrec);
			}
		}
	}

	if (entype & TYPE_TAB) {

		/* modify tab_entry in temp DDB_TAB file */
		if (mod_tabent(devrec.tab, DEV_MOD) < 0) {
		    /* remove temporary DDB_SEC and DDB_DSFMAP 
		     * files, if they were created */
		    rmtmpddb(Tmp_ddbfiles[2]);
		    rmtmpddb(Tmp_ddbfiles[1]);
		    QUIT_PUTDEV(devrec);
		}
	 }

	 /* rename the temp files to DDB filenames */
	 if(update_ddb()<0) {
	 	QUIT_PUTDEV(devrec);
	 }

	 /* free memory allocated for attributes in devrec */
	 free_devrec(&devrec);


	 /* Unlock the Device Database */
	 unlock_ddb();

	 /* return warning to invoking command 
	  * so that appropriate exit status is returned
	  */
	 if (warning)
		return(WARNING);

	/* Fini */
	return(SUCCESS);
}

/*
 *  int remdevrec(device, ddblock)
 *	char   *device
 *	int 	ddblock
 *
 *	This function removes the specified <device> from the Device
 *	Database(DDB). The <device> could be an alias name
 *	or an absolute pathname to a device special file.
 *	If <device> is a pathname of dsf, then the alias to which the
 *	dsf maps is removed.
 *
 *  Arguments:
 *	device		an alias name or absolute pathname to dsf.
 *	ddblock		flag that tells the function if the DDB has been lock.
 *			The flag is implicitly used to determine which command
 *			called the function
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 */

int
remdevrec(char *device, int ddblock)
{
	sec_entry		*sec;		/* ptr to sec_entry     */
	dsf_entry		*dsf;		/* ptr to dsf_entry     */
	tab_entry		*tab;		/* ptr to tab_entry     */
	char			alias[DDB_MAXALIAS],  /* alias name        */
				salias[DDB_MAXALIAS]; /* secure alias name */
	int			dtype;		/* dsf type             */
	int			atype;		/* alias type           */
	int			err, mapcnt, tabinfo;
	int			tabversion = gettabversion();

	/* Make sure that __tabversion__ is initialized */
	if (__tabversion__ == 0) {

		__tabversion__ = gettabversion();

		if (__tabversion__ == FAILURE) {
	    		ddb_errmsg(SEV_ERROR,EX_ACCESS,E_ACCESS);
			return(FAILURE);
		}
	}

	/* Validate the device alias */
	if (!(valid_alias(device) || valid_path(device))) {
	    ddb_errmsg (SEV_ERROR, EX_ERROR, E_DINVAL, device);
	    return(FAILURE);
	}

	/* DDB is not lock */
	if (ddblock != TRUE) {
		/* Command invoking function must be ddbconv(1M) */
		strcpy(Cmdname, DDBCONV);

		/* Lock the Device Database */
		if (lock_ddb() != SUCCESS) {
	    		/* unable to lock Device Database */
	    		switch (ddb_errget()) {
			case (EAGAIN):
		    		ddb_errmsg(SEV_ERROR, EX_ERROR, E_DDBUSE);
		    		break;
			default:
		    		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    		break;
	    		}
	    		return(FAILURE);
		}
	} 

	if (valid_path(device)) {

		/* get the alias name that <device> maps to */
		if (__tabversion__ == __4ES__) {
			if (getdsfmap(device, &dtype, alias, salias) < 0) {
		    		ddb_errmsg(SEV_ERROR,EX_ENODEV,E_NODEV,device);
		    		unlock_ddb();
		    		return(FAILURE);
			}
		} else {
			tab = getalias(device,&dtype);
			if (tab->alias == (char *)NULL) {
		    		ddb_errmsg(SEV_ERROR,EX_ENODEV,E_NODEV,device);
		    		unlock_ddb();
		    		return(FAILURE);
			}
			strcpy(alias,tab->alias);
		}

	} else {
		/* <device> should be an alias name */
		strcpy(alias, device);
	}

	/* Make sure that the device is already in DDB */
	if ((tabinfo=getaliasmap(alias,&atype,&mapcnt,salias))<0) {
		/* DDB files could not be accessed */
		ddb_errmsg (SEV_ERROR, EX_ACCESS, E_ACCESS);
		unlock_ddb();
		return(FAILURE);
	} else if (tabinfo == 0) {
		/* The alias does not exist in DDB */
		ddb_errmsg(SEV_ERROR, EX_ENODEV, E_NODEV, alias);
		unlock_ddb();
		return (FAILURE);
	}

	/* cannot remove device entry, if it is being mapped to *
	 * by other logical device aliases in the DDB           */
	if (_mac_installed()) {
		if ((atype==DEV_SECDEV)&&(mapcnt>0)) {
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_SECENT, alias);
		    unlock_ddb();
		    return(FAILURE);
		}
	}

	err = FALSE;
	/* remove the entry defined for alias 
	 * from the appropriate Device Database files */

	/* create new magic number */
	cr_magicno();

	/* remove entry from DDB_TAB file */
	if (rem_tabent(alias) < 0) {
		err = TRUE;
	}

	if (__tabversion__ == __4ES__) {

		/* remove entry from DDB_SEC file */
		if ((!err) && (_mac_installed()) && (rem_secent(alias) < 0)) {
			rmtmpddb(Tmp_ddbfiles[0]);
			err = TRUE;
		}
	
		/* remove entry from DDB_DSFMAP file */
		if ((!err) && (rem_dsfent(alias)<0)) {
			rmtmpddb(Tmp_ddbfiles[0]);
			rmtmpddb(Tmp_ddbfiles[1]);
			err = TRUE;
		}
	}	

	/* if error encountered, unlock ddb */
	if (err) {
		unlock_ddb();
		return (FAILURE);
	}

	/* replace DDB files with temp files */
	if(update_ddb() < 0) {
		unlock_ddb();
		return(FAILURE);
	}

	unlock_ddb();

	/* Fini */
	return(SUCCESS);
}

/*
 *  int rmdevattrs(device, attrs, ddblock)
 *	char   *device
 *	char  **attrs
 *	int	ddblock
 *
 *	This function removes the specified attributes from the specified
 *	<device> in the Device Database(DDB). The <device> could be an alias
 *	name or an absolute pathname to a device special file.
 *	If <device> is a pathname of dsf, then the alias to which the
 *	dsf maps is used for modification. 
 *
 *  Arguments:
 *	device		an alias name or absolute pathname to dsf.
 *	attrs		The pointer to the first item of a list of attributes
 *			to be removed from the DDB, for specified <device>.
 *			(This value may be (char **) NULL).
 *	ddblock		flag that tells the function if the DDB has been lock.
 *			The flag is implicitly used to determine which command
 *			called the function
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 */

int
rmdevattrs(char *device, char **attrs, int ddblock)
{
	dev_record		devrec;		/* dev_record structure */
	tab_entry		*tab;
	char			alias[DDB_MAXALIAS],  /* alias name        */
				salias[DDB_MAXALIAS]; /* secure dev alias  */
	int			entype;		/* entry type of attr   */
	int			dtype;		/* dsf type             */
	int			atype;		/* alias type           */
	int			tabinfo, mapcnt, sattrs;
	int			ret = SUCCESS; 
	char			*aptr;
	int			noattr = 0;	/* flag for undefined
						 * attributes */


	/* Make sure that __tabversion is initialized */
	if (__tabversion__ == 0) {

		__tabversion__ = gettabversion();

		if (__tabversion__ == FAILURE) {
	    		ddb_errmsg(SEV_ERROR,EX_ACCESS,E_ACCESS);
			return(FAILURE);
		}
	}

	/* Validate the device alias */
	if (!(valid_alias(device) || valid_path(device))) {
	    ddb_errmsg (SEV_ERROR, EX_ERROR, E_DINVAL, device);
	    return(FAILURE);
	}

	/* Lock the Device Database, if it hasn't been lock by
	 * the invoker. This check is done to be consistent with the other
	 * functions that lock DDB, since this function is only invoked
	 * by putdev(1M) which locks the DDB before calling this function. */
	if (ddblock != TRUE) {
		if (lock_ddb() != SUCCESS) {
	    		/* unable to lock Device Database */
	    		switch (ddb_errget()) {
			case (EAGAIN):
		    		ddb_errmsg(SEV_ERROR, EX_ERROR, E_DDBUSE);
		    		break;
			default:
		    		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    		break;
	    		}
	    		return(FAILURE);
	       }
	}

	if (valid_path(device)) {

		/* get the alias name <device> maps to */

		if (__tabversion__ == __4ES__) {
			if (getdsfmap(device, &dtype, alias, salias) < 0) {
		    		ddb_errmsg(SEV_ERROR,EX_ENODEV,E_NODEV,device);
		    		unlock_ddb();
		    		return(FAILURE);
			}
		} else {
			tab = getalias(device,&dtype);
			if (tab->alias == (char *)NULL) {
		    		ddb_errmsg(SEV_ERROR,EX_ENODEV,E_NODEV, device);
		    		unlock_ddb();
		    		return(FAILURE);
			}
			strcpy(alias,tab->alias);
		}
	} else {
		/* <device> should be an alias name */
		strcpy(alias, device);
	}

	/* Make sure that the device is already in DDB */
	if ((tabinfo=getaliasmap(alias,&atype,&mapcnt,salias))<0) {
		/* DDB files could not be accessed */
		ddb_errmsg (SEV_ERROR, EX_ACCESS, E_ACCESS);
		unlock_ddb();
		return(FAILURE);
	} else if (tabinfo == 0) {
		/* The alias does not exist in DDB */
		ddb_errmsg(SEV_ERROR, EX_ENODEV, E_NODEV, alias);
		unlock_ddb();
		return (FAILURE);
	}

	/*
	 * Extract attr names and values from <attrval> attr list
	 *  - determine attr type; which devrec field it belongs in
	 * 	- validate each attr value
	 *  - initialize corresponding entry field
	 */
	if ((entype=make_devrec(alias, attrs, DEV_REM, &devrec))<=0) {
		unlock_ddb();
		return(FAILURE);
	}

	if (_mac_installed()) {

		if ((atype==DEV_SECDEV)&&(entype & TYPE_SEC)) {
		    sattrs = (devrec.sec->range)||(devrec.sec->state)||
					(devrec.sec->mode);

		    if (mapcnt > 0) {
			/* secure alias being mapped to by logical aliases *
			 * Cannot remove essential security attrs.         */
			if (sattrs) {
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_SECENT, alias);
			    unlock_ddb();
			    return(FAILURE);
			}
		    } else {
			/* Secure alias not being mapped to by any alias. *
			 * Must remove all essential sec. attrs together. */
			if ((sattrs) &&
			    (!((devrec.sec->range)&&(devrec.sec->state)&&
				(devrec.sec->mode)))) {
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSTOG, alias);
			    unlock_ddb();
			    return(FAILURE);
			}
		    }
		} else {
			/* if alias is not a secure alias, and within the
			 * attributes requested for removal there are 
			 * security attribute, issue error.
			 */
			if (atype == DEV_ALIAS && (entype & TYPE_SEC)) {
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSSEC, alias);
			    unlock_ddb();
			    return(FAILURE);
			}
		}
	}
	/*
	 * Remove specified attrs from respective DDB files
	 */

	/* create new magic number */
	cr_magicno();

	/* security attributes */
	if (entype & TYPE_SEC) {

		/* check first that ES is installed */
		if (!_mac_installed()) {
		 	ddb_errmsg(SEV_ERROR, EX_NOPKG, E_NOPKG, Cmdname);
			QUIT_PUTDEV(devrec);
		}

		if (aptr=(char *)malloc(strlen(alias)+1)) {
	        	strcpy(aptr,alias);
	           	devrec.sec->alias = aptr;
		} else {
			unlock_ddb();
		    	ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		    	return (FAILURE);
	        }

		/* remove attrs from sec_entry in temp DDB_SEC file */
		ret = mod_secent(devrec.sec, DEV_REM);
		if (ret < 0) {
		 	QUIT_PUTDEV(devrec); 
		} else {
			if (ret == NOATTR)
				noattr++;
		}
	}

	/* clear error buffer before calling mod_dsfent */
	ddb_errset(0);

	if (__tabversion__ == __4ES__) {

		/* entype will be set to TYPE_DSF if the attributes are
		 * cdevice or bdevice. But if the DDB_TAB is a 4.0 table
		 * they will be handled by mod_tabent
		 */
		if (entype & TYPE_DSF) {
	
			/* remove attrs from dsf_entry */
			ret = mod_dsfent(devrec.dsf, alias, salias, DEV_REM);
			if (ret < 0) {
				/* remove temp DDB_SEC file,
				 * if the file was created. */
		    		rmtmpddb(Tmp_ddbfiles[2]);
		    		QUIT_PUTDEV(devrec);
			} else {
				if (ret == NOATTR)
					noattr++;
			}
		}
	}

	/* clear error buffer before calling mod_tabent */
	ddb_errset(0);

	if (entype & TYPE_TAB) {

		/* remove attrs from tab_entry in temp DDB_TAB file */
		ret = mod_tabent(devrec.tab, DEV_REM);
		if (ret < 0) {
		    /* remove temporary  DDB_SEC and DDB_DSFMAP 
		     * files, if they were created */
		    rmtmpddb(Tmp_ddbfiles[2]);
		    rmtmpddb(Tmp_ddbfiles[1]);
		    QUIT_PUTDEV(devrec);
		} else {
			if (ret == NOATTR)
				noattr++;
		}
	}

	/* rename the temp files to DDB filenames */
	if (update_ddb()) {
		QUIT_PUTDEV(devrec);
	}

	/* free memory allocated for attributes in devrec */
	free_devrec(&devrec);

	/* Unlock the Device Database */
	unlock_ddb();

	/* Fini */
	if (noattr)
		return(NOATTR);
	return(SUCCESS);
}

/*
 *  int appattrlist()
 *
 *	This function appends the list of values to the specified
 *	attribute, that takes value-lists.
 *
 *	The attributes that take value lists are pre-defined in the
 *	the static array - Val_list[]. Therefore, if and when a
 *	new attribute is defined, that takes a comma separated list
 *	of values, that attribute MUST be added to the Val_list[] array.
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 */

int
appattrlist(device, attrval, ddblock)
char	*device;
char	*attrval;
int 	ddblock;
{
	dev_record	devrec;		/* device record read from DDB */
	char		*attr, *value;	/* temp ptrs to attr-value str */
	char		*next;		/* temp ptr to next value in list*/
	char		*range, *state, *mode;
	char		*modstr[2]={(char *)NULL, /* modified attr-value */
				(char *)NULL };
	int		listtype;
	essentials	*alias;


	/* extract attr name from input string */
	if ((attr=getfield(attrval,"=",&value))==(char *)NULL) {
	    ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
	    return(FAILURE);
	}

	/* verify that list contains one or more items */
	if (getlistcnt(value)<1) {
	    ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
	    return(FAILURE);
	}
		
	/*
	 * check if input attr, is a valid attribute name that
	 * takes value-list (appears in Val_list[])
	 */
	listtype = 0;
	while (listtype<MAXVAL_LIST) {
	    if (strcmp(attr,Val_list[listtype])==0) 
		break;
	    listtype++;
	}

	if (listtype == MAXVAL_LIST) {
	    	/* Should define a NEW error message */
	    	ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
	    	return(FAILURE);
	} 

	/* get all the attrs for specified device from DDB */
	if (get_devrec(device, &devrec) == FAILURE) {
	    switch(ddb_errget()) {
		case(EINVAL):
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_DINVAL, device);
		    return(FAILURE);
		    break;
		case(ENODEV):
		    ddb_errmsg(SEV_ERROR, EX_ENODEV, E_NODEV, device);
		    return(FAILURE);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    return(FAILURE);
		    break;
	    }
	}

	
	/* If users, verify that device is a secure device */
	if (listtype == USERS) {

		/* If modifying users for alias that maps
		 * to another alias, that's an error
		 */
		if (strcmp(devrec.tab->secdev, devrec.tab->alias) != 0) {
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSSEC,
				devrec.tab->alias);
			return(FAILURE);
		} 

		/* if the alias == secdev, make sure 
		 * the alias is a secure alias */

		alias = getessentials(device);
		if (!(MINSECATTRS(alias->range,alias->state,alias->mode))) {
			ddb_errmsg(SEV_ERROR, EX_ERROR,E_ESSSEC,devrec.tab->alias);
			return(FAILURE);
		} 

	}

	/* append new list of values in "value" to the end of  *
	 * existing value-list defined for attr in devrec      */
	switch(listtype) {

	case(CDEVLIST):

	    /* DDB_CDEVLIST to be modified */
	    if (modstr[0]=(char *)malloc(strlen(devrec.dsf->cdevlist)+
				strlen(value)+strlen(DDB_CDEVLIST)+2)) {
		*modstr[0] = '\0';
		strcat(modstr[0], DDB_CDEVLIST);
		strcat(modstr[0], "=");
		if (devrec.dsf->cdevlist) {
		    strcat(modstr[0], devrec.dsf->cdevlist);
		    strcat(modstr[0], ",");
		}
		strcat(modstr[0], value);	
	    } else {
		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    }
	    break;

	case(BDEVLIST):

	    /* DDB_BDEVLIST to be modified */
	    if (modstr[0]=(char *)malloc(strlen(devrec.dsf->bdevlist)+
				strlen(value)+strlen(DDB_BDEVLIST)+2)) {
		*modstr[0] = '\0';
		strcat(modstr[0], DDB_BDEVLIST);
		strcat(modstr[0], "=");
		if (devrec.dsf->bdevlist) {
		    strcat(modstr[0], devrec.dsf->bdevlist);
		    strcat(modstr[0], ",");
		}
		strcat(modstr[0], value);	
	    } else {
		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    }
	    break;

	case(USERS):

	    /* DDB_USERS to be modified */

	    if (modstr[0]=(char *)malloc(strlen(devrec.sec->users)+
				strlen(value)+strlen(DDB_USERS)+2)) {
		*modstr[0] = '\0';
		strcat(modstr[0], DDB_USERS);
		strcat(modstr[0], "=");
		if (devrec.sec->users) {
		    strcat(modstr[0], devrec.sec->users);
		    strcat(modstr[0], ",");
		}
		strcat(modstr[0], value);	
	    } else {
		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    }
	    break;

	default:
	    break;
	}

	/* modify the specified device attr with new value-string */
	if (moddevrec(device, modstr, ddblock) < 0) {
	    /* attempt failed, return FAILURE */
	    free_devrec(&devrec);
	    return(FAILURE);
	}
	/* free memory allocated for devrec */
	free_devrec(&devrec);
	return(SUCCESS);
}

/*
 *  int remattrlist()
 *
 *	This function removes the list of values from the specified
 *	attribute, that takes value-lists.
 *
 *	The attributes that take value lists are pre-defined in the
 *	the static array - Val_list[]. Therefore, if and when a
 *	new attribute is defined, that takes a comma separated list
 *	of values, that attribute MUST be added to the Val_list[] array.
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 */

int
remattrlist(device, attrval, ddblock)
char	*device;
char	*attrval;
int 	ddblock;
{
	dev_record	devrec;		/* device record read from DDB */
	char		*attr, *value;	/* temp ptrs to attr-value str */
	char		*modstr[2];
	char		*oldstr, *remstr, *next;
	int		listtype ;
	int 		rem_cnt = 0; /* Total number of items being removed */
	uid_t 		uid;
  	char 		user[LOGNAMEMAX], *perm;
	char 		*newlist;

	modstr[0] = modstr[1] =	(char *)NULL; 

	
	/* extract attr name from input string */
	if ((attr=getfield(attrval,"=",&value))==(char *)NULL) {
	    ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
	    return(FAILURE);
	}

	/*
	 * Check if input attr, is a valid attribute name that
	 * takes value-list (appears in Val_list[])
	 */
	listtype = 0;
	while (listtype<MAXVAL_LIST) {
	    if (strcmp(attr,Val_list[listtype])==0) 
		break;
	    listtype++;
	}

	if (listtype==MAXVAL_LIST) {
	    /* Should define a NEW error message */
	    ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
	    return(FAILURE);
	}


	/* get all the attrs for specified device from DDB */
	if (get_devrec(device, &devrec) == FAILURE) {
	    switch(ddb_errget()) {
		case(EINVAL):
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_DINVAL, device);
		    break;
		case(ENODEV):
		    ddb_errmsg(SEV_ERROR, EX_ENODEV, E_NODEV, device);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    break;
	    }
	    return(FAILURE);
	}

	next = value;

	/* get number of items being removed */
	rem_cnt = getlistcnt(value); 
	if (!rem_cnt) {
	    	ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
		return(FAILURE);
	}

	/* Removing items from list */
	switch(listtype) {
	case(CDEVLIST):

		/* Check if attribute is defined. If it isn't
		 * set error buffer and return to invoking function
		 */
		if (devrec.dsf->cdevlist == (char *)NULL) {
		   	ddb_errmsg(SEV_ERROR, EX_NOATTR, E_NOATTR,
				DDB_CDEVLIST,devrec.tab->alias);
		    	free_devrec(&devrec);
		    	return(FAILURE);
	  	}

		/* Allocate memory for the modstr[0] which
		 * will contain the final list of items.
		 */
	  	modstr[0]=(char *)malloc(strlen(devrec.dsf->cdevlist)+
				strlen(DDB_CDEVLIST)+2);

		if (modstr[0] == (char *)NULL) {
	    		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			free_devrec(&devrec);
			return(FAILURE);
	  	}

		strcpy(modstr[0], DDB_CDEVLIST);
	    	strcat(modstr[0], "=");

		/* oldstr poinsts to the list of 
		 * existing values for attribute cdevlist
		 */
	    	oldstr = devrec.dsf->cdevlist;

		while (remstr = getfield(next,",",&next)) {
			/* list of values being removed 
			 * has more than one item */

			/* search for pathname to be removed
			 * in old attr-value str */
	
			if (valueinlist(remstr,oldstr,&newlist) == FAILURE) {
				ddb_errmsg(SEV_ERROR, EX_INVAL, E_INVAL,
					DDB_CDEVLIST);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}

			/* update the oldstr */
			oldstr = newlist;
			rem_cnt--;
	    	}

		/* Need to proccess the only or the last item in the list
		 * of values being removed. The only or last item in the list
		 * is not found by getfield with delimiter ",".
		 */
		if (rem_cnt) {

	    		if (valueinlist(next,oldstr,&newlist) == FAILURE) {
				ddb_errmsg(SEV_ERROR, EX_INVAL, E_INVAL,
					DDB_CDEVLIST);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}
		}

	    	strcat(modstr[0], newlist);
		break;

	case(BDEVLIST):

		/* Check if attribute is defined. If it isn't
		 * set error buffer and return to invoking function
		 */
		if (devrec.dsf->bdevlist == (char *)NULL) {
			ddb_errmsg(SEV_ERROR, EX_NOATTR, E_NOATTR, 
				DDB_BDEVLIST,devrec.tab->alias);
		    	free_devrec(&devrec);
		    	return(FAILURE);
	  	}

		/* Allocate memory for the modstr[0] which
		 * will contain the final list of items.
		 */
	  	modstr[0]=malloc(strlen(devrec.dsf->bdevlist)
			+strlen(DDB_BDEVLIST)+2);

	  	if (modstr[0] == (char *)NULL) {
	  		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			free_devrec(&devrec);
			return(FAILURE);
	  	}

	  	strcpy(modstr[0], DDB_BDEVLIST);
	  	strcat(modstr[0], "=");
	
	  	oldstr = devrec.dsf->bdevlist;

	  	while (remstr = getfield(next,",",&next)) {
			/* list of values being removed 
			 * has more than one item */

			/* search for pathname to be removed 
			 * in old attr-value str */
			if (valueinlist(remstr,oldstr,&newlist) == FAILURE) {
				ddb_errmsg(SEV_ERROR, EX_INVAL, E_INVAL,
					DDB_BDEVLIST);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}
			oldstr = newlist;
			rem_cnt--;
		}

		/* Need to proccess the only or the last item in the list
		 * of values being removed. The only or last item in the list
		 * is not found by getfield with delimiter ",".
		 */
		if (rem_cnt) {

	    		if (valueinlist(next,oldstr,&newlist) == FAILURE) {
				ddb_errmsg(SEV_ERROR, EX_INVAL, E_INVAL,
					DDB_BDEVLIST);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}
		}
	    
	    	strcat(modstr[0], newlist);
	
	  	break;

	case(USERS):

		/* mac must be installed in order to modify
		 * a security attribute */
		if (!_mac_installed()) {
			 ddb_errmsg(SEV_ERROR, EX_NOPKG, E_NOPKG, Cmdname);
			 free_devrec(&devrec);
			 return(FAILURE);
		}
		
		/* If modifying users, the alias must be a secure alias */
		if (strcmp(devrec.tab->alias,devrec.tab->secdev) != 0) {
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_ESSSEC,
				devrec.tab->alias);
			free_devrec(&devrec);
			return(FAILURE);
		} 
		
		/* Check that attribute users has a value */
		if ((devrec.sec == (sec_entry *)NULL)||
		    (devrec.sec->users == (char *)NULL) ) {
		  	ddb_errmsg(SEV_ERROR, EX_NOATTR, E_NOATTR,
					DDB_USERS,devrec.tab->alias);
		  	free_devrec(&devrec);
		  	return(FAILURE);
		}
		
		/* Allocate memory for the modstr[0] which
		 * will contain the final list of items.
		 */
		modstr[0]=(char *)malloc(strlen(devrec.sec->users)+
		    strlen(DDB_USERS)+2);
		if ( modstr[0] == (char *)NULL) {
	  		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			free_devrec(&devrec);
			return(FAILURE);
		}
		
		strcpy(modstr[0], DDB_USERS);
		strcat(modstr[0], "=");
		oldstr = devrec.sec->users;
		
		/* remove specified users in UAL */
		while (remstr = getfield(next,",",&next)) {

			/*  more than one item in the list */
		
			/* get login and translate it to an uid */
			remstr = getfield(remstr, ">", &perm);
			if (parse_uid(remstr,&uid) == FAILURE) {
				/* the uid was invalid.  */
		  		ddb_errmsg(SEV_ERROR, EX_ERROR, E_INUID,
						DDB_USERS, remstr);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}

			/* Create string that consists of "uid>permission"
		 	 * Compared this string with device's users's list */
			sprintf(user,"%d",uid);
			strcat(user,">");
			strcat(user,perm);

			/* search for pathname to be removed 
			 *in old attr-value str */
			if (valueinlist(user,oldstr,&newlist) == FAILURE) {
				ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL,
					DDB_USERS);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}
			rem_cnt++;
			oldstr = newlist;
		}

		if (rem_cnt) {

     	    		/* get login and translate it to uid */
	    		remstr = getfield(next, ">", &perm);
	    		if (parse_uid(remstr,&uid) != FAILURE) {

				/* Create string that consists of 
				 * "uid>permission". Compare this
				 * string with device's users's list */
				sprintf(user,"%d",uid);
				strcat(user,">");
				strcat(user,perm);

	    			if (valueinlist(user,oldstr,&newlist) == FAILURE) {
					ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL,
					DDB_USERS);
					free_devrec(&devrec);
					free(modstr[0]);
					return(FAILURE);
				}
			} else {
				/* Invalid uid/logname */
		  		ddb_errmsg(SEV_ERROR, EX_ERROR, E_INUID,
						DDB_USERS, remstr);
				free_devrec(&devrec);
				free(modstr[0]);
				return(FAILURE);
			}

		}
	  	strcat(modstr[0], newlist);
	
  	  	break;

	default:
		/* can't get here because the code checks the validity
		 * of listtype before getting into this switch statement
		 */
	  	break;
	}

	/* If after deleting the specified values from the list, the
	 * list is empty, instead of calling moddevrec(), call
	 * rmdevattrs() to delete the attribute from the device's entry
	 */
	if (*newlist == '\0') {
		strcpy(modstr[0],Val_list[listtype]);
		if(rmdevattrs(device,modstr,ddblock) < 0) {
	    		/* attempt failed, return FAILURE */
	    		free_devrec(&devrec);
			free(modstr[0]);
	    		return(FAILURE);
		}
	} else {
		/* modify the specified device attr with new value-string */
		if (moddevrec(device, modstr, ddblock) < 0) {
	    		/* attempt failed, return FAILURE */
	    		free_devrec(&devrec);
			free(modstr[0]);
	    		return(FAILURE);
		}
	}

	free_devrec(&devrec);
	free(modstr[0]);

	return(SUCCESS);
}
/*
 *  int rmsecdevs(vflag)
 *
 *	This function removes the <secdev> attribute value for
 *	each of the aliases in DDB_TAB file, and each of the dsfs
 *	defined in DDB_DSFMAP file.
 *	<vflag> indicates verbose format of output.
 *	If vflag = TRUE, the device aliases will be printed.
 *		   FALSE, nothing will be printed.
 *
 *  Returns:  int
 *	SUCCESS 	if successful
 *	FAILURE 	with ddbmsg buffer setup with appropriate error.
 */

int
rmsecdevs(vflag)
int	vflag;
{
	FILE		*fp, *tmpfp;	/* file pointers               */
	char		*tmpddb;	/* ptr to temp DDB filename    */
	char		*next,*skipstr,	
			*rest;		 /* ptr to next field           */
	char		*rec;		/* curr record in DDB file     */
	char		*recalias;	/* alias in curr record        */
	char		savealias[DDB_MAXALIAS];
	char		buf[80];	/* where file's first line is stored
					 * This value is not used. 	*/

	int		err;		/* error flag                  */
	int		i;

	/* Lock the Device Database */
	if (lock_ddb() == FAILURE) {
	    /* unable to lock Device Database */
	    switch (ddb_errget()) {
		case (EAGAIN):
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_DDBUSE);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    break;
	    }
	    return(FAILURE);
	}

	err = FALSE;

	/* create new magic number */
	cr_magicno();

	/* create temporary DDB_TAB file, open for write */
	if ((tmpfp = opentmpddb(DDB_TAB, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_TAB file */
	    return(FAILURE);
	}
	setmagicno(tmpfp);	/* set new magic number   */

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_TAB file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* skip magic no */
	if (fgets(buf, 80, fp) == (char *)NULL) {
	        /* error reading DDB_TAB file */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}


	/* copy records from DDB_TAB to temp DDB_TAB file.    *
	 * after NULLing secdev attribute field               */
	while (rec = read_ddbrec(fp)) {

		/* write to temp device.tab file comments and empty lines */
		if (*rec == '#' || isspace(*rec)) {
			/* write comments and empty lines to temporary file */
			if (write_ddbrec(tmpfp, rec) < 0) {
				err = TRUE;
				free(rec);
				break;
			}
			continue;
		}
		/* get alias field from rec */
		recalias = getfield(rec,":",&skipstr);

		/* save it so that it can be compared against the 
		 * secdev found and so that it can be printed.
		 */
		strcpy(savealias,recalias);

		*(skipstr-1)=':';	/* replace ':' in record */

		/* skip cdevice, bdevice, and pathname */
		for(i=0 ; i<3;i++) {
			if((skipstr=skpfield(skipstr,":")) == (char *)NULL ) {
				free(rec);
				err = TRUE;
				break;
			}
		}

		/* If skipstr is pointing to :, that means that
		 * the secdev field has been removed,
		 * so don't do anything
		 */
		if (*skipstr != ':' ) {

			/* next points to secdev */
			next = getfield(skipstr,":",&rest);

			if (valid_alias(next) == TRUE) {

				/* add : before the oam attributes. This
			 	 * step basically writes over the existing
				 * secdev field */
				*next = ':';

				/* point to next character in secdev and then
			 	 * null the string out which basically 
			 	 * removes the secdev field
			 	 */
				next++;

				*next = '\0';
	
				/* add the rest of the record  */
				strcat(rec,rest);
	
				/* if verbose, display name of alias for which
			 	* security attributes were removed 
			 	*/
				if (vflag) {
					printf("%s\n", savealias);
				}

			}
		} 

		/* copy modified record to temp file */
		if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			break;
		}
		free(rec);	/* free mem alloc'ed for old rec */

	} /* end while */

	/* if errors in reading DDB_TAB file */
	if (ddb_errget()) {
		/* internal error, delete temp DDB_TAB file */
	    	fclose(tmpfp);
		rmtmpddb(tmpddb);
	    	ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
	    	return(FAILURE);
	}

	/* close DDB_TAB file and temp file */
	fclose(fp);
	fclose(tmpfp);
	    
	/* create temporary DDB_DSFMAP file, open for write */
	if ((tmpfp = opentmpddb(DDB_DSFMAP, &tmpddb)) == (FILE *)NULL) {
		/* error, cannot create temp DDB_DSFMAP file */
	    	return(FAILURE);
	}

	/* set new magic number   */
	setmagicno(tmpfp);	

	/* open DDB_DSFMAP file for read only */
	if ((fp = fopen(DDB_DSFMAP, "r")) == (FILE *)NULL) {
		/* error, cannot open DDB_DSFMAP file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* skip magic no */
	if (fgets(buf, 80, fp) == (char *)NULL) { 
		/* error reading DDB_DSFMAP file */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* copy records from DDB_DSFMAP to temp DDB_DSFMAP file.    *
	 * after NULLing secdev attribute field               */
	while (rec = read_ddbrec(fp)) {

		    /* skip dsf, dsf_type, alias fields of each dsf entry */
		    next = rec;

		    for (i=0 ; i<3 ; i++) {
			if ((next=skpfield(next,":"))==(char *)NULL) {
			    ddb_errmsg(SEV_ERROR, EX_CONSTY, E_CONSTY);
			    rmtmpddb(tmpddb);
			    return(FAILURE);
			}
		    }

		    /* NULL out the secdev field, and terminate *
		     * record with carriage return('\n')        */
		    *next = '\n';
		    *(++next) = '\0';

		    /* copy modified record to temp file */
		    if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			break;
		    }

		    free(rec);	/* free mem alloc'ed for old rec */
	} /* end while */

	/* if errors in reading DDB_DSFMAP file */
	if (ddb_errget()) {
		    /* internal error, delete temp DDB_DSFMAP file */
		    fclose(tmpfp);
		    rmtmpddb(tmpddb);
	    	    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
	    	    return(FAILURE);
	}

	/* close DDB_DSFMAP file and temp file */
	fclose(fp);
	fclose(tmpfp);
	    
	/* rename the temp files to DDB filenames */
	if (update_ddb()) {
		unlock_ddb();
		return(FAILURE);
	}

	/* Unlock the Device Database */
	unlock_ddb();

	/* Fini */
	return(SUCCESS);
}

/* 
 * int setddbinfo(char *ddbfile, struct stat f_info)
 * sets the uid, gid, and mac level of new DDB file "ddbfile",
 * using the information from the original DDB_TAB file.
 * Arguments
 *	ddbfile	contains the path to the file being modified
 *	f_info  contains the uid, and gid to which ddbfile will be set
 *	
 * This function returns
 *		FAILURE if any of the system calls fails and 
 *			sets ddb_errmsg appropriately
 *		SUCCESS otherwise.
 */

int 
setddbinfo(ddbfile, f_info)
char *ddbfile;
struct stat f_info; 
{

	if (__tabversion__ == __4ES__) {

		/* If ltdb is installed, set mac level */
		if (_mac_installed() == TRUE) {
			if ((lvlfile(ddbfile,MAC_SET,&f_info.st_level))== -1){
				ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
				return(FAILURE);
			}
		}

	}

        /* set DAC ownership */
	if ( (chown(ddbfile, f_info.st_uid, f_info.st_gid)) == -1) {
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		return(FAILURE);
	}

	return(SUCCESS);

}
/* 
 * static essentials * getessentials(char *alias)
 * Checks if the alias is a secdev. If it is, the function
 * gets the value of the essential security attributes: range,state,mode
 * and returns then on the structure essentials. Otherwise it returns NULL.
 */
static essentials * 
getessentials(alias)
char * alias;
{
	essentials * pdev_esssec;
	char 	*aptr;
	pdev_esssec = (essentials *)malloc(sizeof(essentials));

	pdev_esssec->range = (char *)NULL;
	pdev_esssec->mode = (char *)NULL;
	pdev_esssec->state = (char *)NULL;

	/* if alias is not defined in the DDB, returned now */
	aptr = devattr(alias,DDB_SECDEV);
	if (aptr == (char *)NULL)
		return(pdev_esssec);

	pdev_esssec->range =  devattr(alias, DDB_RANGE);
	pdev_esssec->mode  =  devattr(alias, DDB_MODE);
	pdev_esssec->state =  devattr(alias, DDB_STATE);
	return(pdev_esssec);
}
/*
 * int
 * valueinlist( char *remvalue, char *list, char **newlist)
 *
 * This function searches for the value passed as the first argument
 * in the list passed as the second argument. If the item is found,
 * it is removed from the list and a new list is returned in
 * the third argument. 
 *
 * The function returns SUCCESS if the item is found in the list
 * or FAILURE is the item is not found.
 * 
 */
int 
valueinlist(char *remvalue, char *list, char **newlist)
{
	char *p; /* points to the current location of list being inspected */
	char *value, *rest;
	char *newstring;
	int len = strlen(list);


	if (list == (char *)NULL) {
		return(FAILURE);
	}

	p = list;
	while (value = getfield(p, ",", &rest)) {

		if (strcmp(value, remvalue) != 0) {

			/* value being removed and item in list don't match, 
			 * so put back the "," that was removed by getfield
			 */
			*(rest - 1) = ',';
			p = rest;
			continue;
		}

		/* Value was found */

		if (p == list) {

			/* value is the first one in the  list */
			*newlist = rest;
		}
		
		/* Value is in the middle of the list. 
		 * Set the pointer to the item being removed to be NULL */

		*value = '\0'; 

		value = memmove(value,rest,strlen(rest)+1);

		if (*value != '\0' && value != (char *)NULL) {
			*newlist = list;
			return(SUCCESS);
		}

		return(FAILURE);
	}

	/* getfield can't not handle the last item in the field 
	 * or the only item in the field -- no "," in the string.
	 * Check the only or last item in the list */
	if (strcmp(p, remvalue) != 0) {
		return(FAILURE);
	}

	if (p == list) {
		/* item removed is the only in the list*/
		*newlist = (char *)NULL;
		return(SUCCESS);

	}	

	/* Item should be the last on in the list */ 
	if (p != rest)
		/* problems if at this point is not pointing to rest */
		return(FAILURE);

	/* Null the part of the string rest points to*/
	*(rest-1) = '\0';

	*newlist = list;
	return(SUCCESS);
	
}
