/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/ddb_gen.c	1.1.15.3"
#ident	"$Header: ddb_gen.c 1.4 91/06/25 $"

/*LINTLIBRARY*/

#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mac.h>
#include	<devmgmt.h>
#include	"devtab.h"

/*
 *  Functions defined in ddb_gen.c:
 *
 *	lock_ddb()		Locks the Device Database (DDB_DSFMAP file)
 *	unlock_ddb()		Unlocks the Device Database (DDB_DSFMAP file)
 *	read_ddbrec()		Reads the next record from specified DDB file.
 *	write_ddbrec()		Writes the record to specified DDB file.
 *	make_devrec()		Builds dev_record struct with input attr values.
 *	ddb_check()		Checks that the files are consistent
 *	get_devrec()		Gets all attrs of specified device
 *	free_devrec()		Frees memory allocated to dev_record
 *	getmagicno()		Gets magic-no from specified DDB file
 *	setmagicno()		Sets magic-no in specified DDB file
 *	cr_magicno()		Creates magic-no using time(2).
 */


/*
 *  Static data definitions:
 *	Lock_fd		File descriptor of locked DDB file
 *	recbufsz	The size of the buffer used for reading records
 *	recbuf		Addr of malloc() buffer for reading records
 *	xtndcnt		Number of malloc()/realloc() calls on record buffer
 */
static	int		Lock_fd;
static  char		Magic_no[64];

#define RETRY_COUNT	10		/* ddb lock retry count */

static	int		xtndcnt = 0;
static	char	       *recbuf = (char *) NULL;
static	int		recbufsz = 0;

/*
 * int lock_ddb()
 *
 *	Sets an advisory lock on the DDB_TAB file. If it is already
 *	locked, it retries RETRY_COUNT times. If it is still unsuccessful
 *	it returns a negative value of errno. 
 *
 *  Arguments:
 *
 *  Static Global variable:
 *	Lock_fd		File descriptor of locked file.
 *
 *  Returns:  int
 *	SUCCESS		if successful
 *	FAILURE		if function fails to lock DDB file
 *	                ddb_errset() invoked to set error code.
 *			EAGAIN/EACCESS :if DDB_TAB could not be locked.
 *			(According to the 4.0's Programmers Guide ,
 *			 it could be EAGAIN or EACCES if the file is locked
 */

int
lock_ddb()
{
	int		fd;		/* file descriptor           */
	int		retry;		/* lock retry count          */
	int		err;		/* FLAG, TRUE if error       */
	int		olderrno;	/* Old value of "errno"      */

	if ((fd=open(DDB_TAB, O_RDWR)) < 0) {
    		/* could not open DDB_TAB file */
    		ddb_errset(errno);
    		return(FAILURE);
	}

	/*
	 * Lock the DDB_TAB file (for writing).  If it's not
	 * available, retry RETRY_COUNT times. If still unsuccessful
	 * it returns -(errno).
	 */
	retry = RETRY_COUNT;
	err = 0;

	/* Keep retrying until locked or timeout occurs */
	while (lockf(fd, F_TLOCK, 0) < 0) {
		if ((err=errno) == EAGAIN || errno == EACCES) {
		    if (--retry == 0) {
			/* RETRY_COUNT expired; break out of loop */
			break;
		    }
		    /* sleep for 2 seconds before next retry */
		    sleep(2);
		} else {
		    break;
		}
	}
	if (err) {
		/* error encountered while locking */
		close(fd);
		ddb_errset(err);
		return(FAILURE);
	}

	/* set global fp, return success */
	Lock_fd = fd;
	return(SUCCESS);
	
}

/*
 * int unlock_ddb()
 *
 *	Unlocks the locked Device Database 
 *
 *  Arguments:  None
 *
 *  Static Global Variable:
 *	Lock_fd		File descriptor of the Locked file.
 *
 *  Returns:  int
 *	SUCCESS		if successful
 *	FAILURE		errno set by internal to lockf() on failure.
 */

int
unlock_ddb()
{
	int		rtn;		/* return value */

	rtn = SUCCESS;
	/* Unlock previously locked DDB file */
	if (lockf(Lock_fd, F_ULOCK, 0) < 0) {
	    ddb_errset(errno);
	    rtn = FAILURE;
	}
	/* close previously locked file */
	close(Lock_fd);

	/* Finished */
	return(rtn);
}

/*
 *  char *read_ddbrec(fp)
 *	FILE	*fp;
 *
 *	This function reads the next record (char *) at the current offset
 *	from the specified Device Database file, <fp>.
 *
 *  Arguments:	FILE *
 *	fp	Openned file pointer to any one of the DDB files -
 *		DDB_TAB, DDB_SEC, DDB_DSFMAP or a temporary DDB file.	
 *
 *  Returns:	char *
 *	If successful, returns record in malloc'ed memory area.
 *	If it fails, it sets the ddb error code(via ddb_errset()) and
 *      returns NULL.
 */
char *
read_ddbrec(fp)
FILE	*fp;
{
	int	i, j, reclen, recbufsz, xtndcnt;
	char	*recp, *recbuf, *p;

	xtndcnt = 0;
	recbufsz = DDB_BUFSIZ;

	if (recbuf=(char *)malloc(DDB_BUFSIZ)) {
	    if (recp = fgets(recbuf, recbufsz, fp)) {
	    	reclen = strlen(recp);
	    } else {
		free(recbuf);
		return((char *)NULL);
	    }
	} else {
	    ddb_errset(errno);
	    return((char *)NULL);
	}

	while (recp) {
	    if (*(recp+reclen -1) == '\n')  {
		if ((reclen==1) || (*(recp+reclen-2)!='\\')) {
		    break;
		}

	    } else if (reclen == recbufsz-1) {

		/* have we reached max extension count? */
		if (xtndcnt < XTND_MAXCNT ) {
		    p = recbuf;

		    /* expand buffer */
		    if (recbuf = (char *)realloc(recbuf, recbufsz+DDB_BUFINC)) {
			/* update buffer information */
			xtndcnt++;
			recbufsz += DDB_BUFINC;
		    } else {
			/* expansion failed */
			recp = (char *)NULL;
			free(p); /* free old buffer */
			ddb_errset(EFAULT);
			break;
		    }

		} else {

		    /* max extend count exceeded. Insane table?? */
		    recp = (char *)NULL;
		    ddb_errset(EFAULT);
		    free(recbuf);
		    break;
		}
	    } 

	    if (fgets(recbuf+reclen, recbufsz-reclen, fp)) {
		    reclen = strlen(recbuf);
		    recp = recbuf;
	    } else {
		    /* read failed */
		    ddb_errset(errno);
		    free(recbuf);
		    recp = (char *)NULL;
		    break;
	    }
	}	/* end while */
	return(recp);
}

/*
 *  int write_ddbrec(stream, rec)
 *	FILE	*stream;
 *	char	*rec;
 *
 *	This function writes the specified record(char *) to the
 *	specified stream, <stream>, at the current offset.
 *
 *  Arguments:	FILE *
 *	stream	Open file pointer to any one of the DDB files -
 *		DDB_TAB, DDB_SEC, DDB_DSFMAP or a temporary DDB file.	
 *
 *  Returns:	char *
 *	Returns the number of bytes written.
 */
int
write_ddbrec(stream, rec)
FILE	*stream;
char	*rec;
{
	return(fputs(rec, stream));
}

/*
 *  void free_devrec(devrec)
 *	dev_record	*devrec;
 *
 *	This function frees all the memory allocated for each field
 *	entry in the structure pointed by <devrec>.
 *
 *  Arguments:
 *	devrec	output dev_record, with appropriate field initialized.
 *
 *  Returns:	void
 */
void
free_devrec(devrec)
dev_record	*devrec;
{
	if (devrec->tab)	freetabent(devrec->tab);
	if (devrec->sec)	freesecent(devrec->sec);
	if (devrec->dsf)	freedsfent(devrec->dsf);
	devrec->tab = (tab_entry *)NULL;
	devrec->dsf = (dsf_entry *)NULL;
	devrec->sec = (sec_entry *)NULL;
}

/*
 *  int make_devrec(alias, attrval, cmd, devrec)
 *	char		*alias;
 *	char		**attrval;
 *	int		cmd;
 *	dev_record	*devrec;
 *
 *	This function extracts the input attributes and values from
 *	the specified list of <attrval> pairs defined for <alias>, and does
 *	the following for each pair:
 *	    - determines attribute type - TYPE_TAB, TYPE_DSF, TYPE_SEC.
 *		This is determined by calling getattrtype() function.
 *	    - allocates memory for value.
 *	    - invokes appropriate function to initialize 
 *		field entry within <devrec>, based on <cmd>.
 *
 *  Arguments:
 *	alias	input device name
 *	attrval	attr-value pairs of the form attr="value"
 *	cmd	DEV_ADD, DEV_MOD, DEV_REM 
 *	devrec	output dev_record, with appropriate field initialized.
 *
 *  Returns:	int
 *	Returns the type of attributes defined in <attrval>.
 *	If error, it returns FAILURE(-1).
 */
int
make_devrec(alias, attrval, cmd, devrec)
	char		*alias;
	char		**attrval;
	int		cmd;
	dev_record	*devrec;
{
	char		*attr, *value;
	int		etype, entype;
	int		fieldno, rtn;

	/* allocate memory for tab_entry */
	if ((devrec->tab=(tab_entry *)malloc(sizeof(tab_entry))) == NULL) {
	    ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
	    return (FAILURE);
	}

	/* initialize all fields of tab_entry to NULL */
	INIT_TABENTRY(devrec->tab);

	/* initialize alias field of entry */
	if (devrec->tab->alias=(char *)malloc(strlen(alias)+1)) {
		strcpy(devrec->tab->alias,alias);
	} else {
		ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
		return (FAILURE);
	}

	/* allocate memory for sec_entry */
	if ((devrec->sec=(sec_entry *)malloc(sizeof(sec_entry)))==NULL) {
	    free_devrec(devrec);
	    ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
	    return(FAILURE);
	}

	/* initialize all fields of sec_entry to NULL */
	INIT_SECENTRY(devrec->sec);

	if ((devrec->dsf=(dsf_entry *)malloc(sizeof(dsf_entry)))==NULL) {
	    free_devrec(devrec);
	    ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
	    return(FAILURE);
	}

	/* initialize all fields of dsf_entry to NULL */
	INIT_DSFENTRY(devrec->dsf);

	/* set entry type to unknown */
	rtn = SUCCESS;
	entype = TYPE_TAB;

	/* extract next attr=value from attrval list ; 
	 * validate syntax & determine type of attr
	 */
	while ((*attrval)&&(rtn == SUCCESS)) {

	    /* extract attribute  from input attrval */
	    if (cmd == DEV_REM) {
		attr = *attrval;

		if ( (strstr(attr,"=")) != (char *)NULL) {

		    /* Character "=" should not part of an attribute name.
		     * Assuming a syntax error in the command line */
		    free_devrec(devrec);
		    ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
		    return(FAILURE);
		}

	    } else {

		/* cmd = DEV_ADD or DEV_MOD */
		if ((attr=getfield(*attrval,"=",&value))== (char *)NULL) {
		    /* Command usage error, "=" not found */
		    free_devrec(devrec);
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_USAGE);
		    return(FAILURE);
		}
	    }

	    /* determine type of attribute       */
	    etype = getattrtype(attr, &fieldno);

	    switch (etype) {

		case (TYPE_SEC):

		    /* - security attribute */
			
		    /* First check if the Enhanced Security Package is
		     * installed. If it isn't set error buffer with 
		     * E_NOPKG and return with failure */
		    if (! _mac_installed()) {
		    	free_devrec(devrec);
		    	ddb_errmsg(SEV_ERROR, EX_NOPKG, E_NOPKG, Cmdname);
		    	return(FAILURE);
	      	    }

		    /* init field in sec_entry */
		    if (cmd==DEV_REM) {
			rtn = make_secrem(attr,fieldno,devrec->sec);
		    } else {
			rtn = make_secent(attr,value,fieldno,devrec->sec);
		    }
		    break;

		case (TYPE_DSF):

		    	/* - dsf      attribute -  *
		     	 * init field in sec_entry */
		    	if (cmd==DEV_REM) {
				rtn=make_dsfrem(attr,fieldno,devrec); 
		    	} else {
				rtn=make_dsfent(attr,value,fieldno,devrec);
		    	}
		    	break;

		case (TYPE_TAB):

			/* - oam      attribute -  *
		     	 * init field in sec_entry */
		    	if (cmd==DEV_REM) {
				rtn=make_tabrem(attr,fieldno,devrec->tab);
		    	} else {
				rtn=make_tabent(attr,value,fieldno,devrec->tab);
		    	}
		    	break;

		default:
		    /* invalid attr=value pair */
		    free_devrec(devrec);
		    return(FAILURE);
		    break;
	    }

	    if (rtn == SUCCESS) {
		/* valid attribute type found in attrval item */
		entype |= etype;
	    } else {
		/* syntax error in specified value */
		free_devrec(devrec);
		return(rtn);
	    }

	    attrval++;		/* process next attrval in list */

	}    /* end while */
	return(entype);
}	    

/*
 *  int get_devrec(device, devrec)
 *	char		*device;
 *	dev_record	*devrec;
 *
 *	This function gets all the attributes defined for specified
 *	<device>, from the appropriate Device Database files. It returns
 *	the attribute values in <devrec>. Memory for the field entries
 *	in <devrec> is allocated using malloc().
 *
 *  Arguments:
 *	device	input device name or pathname
 *	devrec	output dev_record, with appropriate field initialized.
 *
 *  Returns:	int
 *	SUCCESS	if it succeeds
 *	FAILURE	if alias not found, or any other error encountered.
 */
int
get_devrec(device, devrec)
	char		*device;
	dev_record	*devrec;
{
	char		alias[DDB_MAXALIAS],
			salias[DDB_MAXALIAS];

	int		dtype, tabinfo, mapcnt;
	int 		argflag = 0; 	/* 0 for alias, 1 for pathname */


	if (device == NULL || devrec == NULL)
		return(FAILURE);

 	/* Determine if the argument <device> is an alias  or a pathname */
	if (valid_alias(device)) {
		strcpy(alias, device);

	} else if (valid_path(device)) {
		argflag = 1;

	} else  {
	 	ddb_errset(EINVAL);
	    	return(FAILURE);
	}	

	/* reset all fields of devrec */
	devrec->tab = (tab_entry *)NULL;
	devrec->dsf = (dsf_entry *)NULL;
	devrec->sec = (sec_entry *)NULL;

	if (__tabversion__ == __4ES__) {

		/* If pathname, get the alias name <device> maps to */
		if (argflag) {
	    		if (getdsfmap(device, &dtype, alias, salias) < 0)
				return(FAILURE);
		}

		/* Clear the error buffer */ 
		ddb_errset(0);

		/* get tab_entry of <alias> from DDB_TAB file */
		if ((devrec->tab=get_tabent(alias)) == (tab_entry *)NULL) {
			/* error encountered */
	    		free_devrec(devrec);
	    		return(FAILURE);
		}

		/* check if Enhanced Sec. Package is installed and *
	 	 * and if specified alias is a secure device alias */
 		if (devrec->tab->secdev 
			&& (strcmp(devrec->tab->secdev, alias)==0)) {

			/* yes, get sec_entry of <alias> from DDB_SEC file */
			devrec->sec = get_secent(alias);
			if (devrec->sec == (sec_entry *)NULL) {
				if (ddb_errget()) {
					/* error encountered */
					free_devrec(devrec);
					return(FAILURE);
		    		}
			}
		}

		if ((devrec->dsf=get_dsfent(alias)) == NULL) {
			if (ddb_errget()) {
		    		/* error encountered */
		    		free_devrec(devrec);
		    		return(FAILURE);
			}
		}

	} else {
		/* 4.0 device.tab */

		/* Clear the error buffer */ 
		ddb_errset(0);

		/* If pathname, get the alias name <device> maps to */
		if (argflag) {
			devrec->tab = getalias(device,&dtype);
	    		if (devrec->tab == (tab_entry *)NULL)
				return(FAILURE);
		} else {
			/* get tab_entry of <alias> from DDB_TAB file */
			devrec->tab = get_tabent(alias);
			if (devrec->tab == (tab_entry *)NULL) {
				/* error encountered */
		    		return(FAILURE);
			}
		}
	}
	return(SUCCESS);
}

/*
 *  int ddb_check()
 *
 *	This function checks if all the relevent Device Database files
 *	are present and can be accessed. If all are present, it compares
 *	the Magic Number saved at the start of each file, to see if all
 *	match.
 *
 *	If both the conditions are met, it returns TRUE(1). If all DDB files
 *	are present, but their magic-nos do NOT match, it returns FALSE(0).
 *	However, if some or all of the DDB files are not present or
 *	inaccessible, it returns FAILURE(-1), and sets errno appropriately.
 *
 *  Arguments:
 *
 *  Returns:	int
 *	TRUE	if all DDB files present & magic_no's match.
 *	FALSE	if the magic-nos in DDB files do NOT match.
 *	FAILURE	errno is set as follows:
 *		EACCESS  if a DDB file is not accessible.
 *		ENOENT   if a DDB file is not present.
 *		ENOTDIR  if component of path not a directory.
 *		0        if none of the DDB files are present.  
 */
int
ddb_check()
{
	FILE	*f_tab,*f_dsf,*f_sec;	/* DDB file pointers          */
	char	magictok[128];		/* buffer for magic no's first token */
	unsigned long tabnum, dsfnum, secnum; /* hold the time's number */

	register int ddb_none;		/* TRUE, if DDB files don't exist */
	register int err;		/* errno save area            */

	tabnum = dsfnum = secnum = 0;

	__tabversion__ = gettabversion();

	/* If gettabversion returned FAILURE, then the
	 * device.tab  couldn't be opened or reading from it failed.
	 */
	if (__tabversion__ == FAILURE)
		return(FAILURE);

	ddb_none = FALSE;

	if (__tabversion__ == __4dot0__) {
		if (access(DDB_TAB,R_OK)<0)
			return(FAILURE);
		return(TRUE);
	}

	/* check if DDB_TAB & DDB_DSFMAP accessible */
	/* Open files, compare Magic numbers */
	if (f_tab=fopen(DDB_TAB, "r")) {
	    if (f_dsf=fopen(DDB_DSFMAP, "r")) {
		/* get magic_no's */
		getmagicno (f_tab, magictok, &tabnum);
		getmagicno (f_dsf, magictok, &dsfnum);
		if (tabnum == dsfnum) {
		    fclose(f_tab);
		    fclose(f_dsf);
		} else {
		    /* magic-nos do not match */
		    fclose(f_tab);
		    fclose(f_dsf);
		    return(FALSE);
		}
	    } else {
		err = errno;
		fclose(f_tab);
		errno = err;
		return(FAILURE);
	    }
	} else {
	    if (errno!=ENOENT) {
		/* error, cannot open DDB_TAB file */
		return(FAILURE);
	    } else {
		err = errno;
		if (access(DDB_DSFMAP,R_OK)<0) {
		    if ((errno==ENOENT)&&(err==ENOENT)) {
			/* both DDB_TAB & DDB_DSFMAP not present */
			ddb_none = TRUE;
		    } else {
			errno = err;
			return(FAILURE);
		    }
		} else {
		    errno = err;
		    return(FAILURE);
		}
	    }
	}
	/* if Enhanced Sec., then check if DDB_SEC accessible */
	if (_mac_installed()) {
	    if (f_sec=fopen(DDB_SEC, "r")) {
		/* get magic_no */
		getmagicno (f_sec, magictok, &secnum);
		if (tabnum == secnum) {
		    fclose(f_sec);
		} else {
		    /* magic-nos do not match */
		    return(FALSE);
		}
	    } else {
		if ((errno==ENOENT)&&(ddb_none)) {
		    /* none of the DDB files present */
		    ddb_none = TRUE;
		} else {
		    /* error, cannot access DDB_SEC file */
		    return(FAILURE);
		}
	    }
	}
	if (ddb_none) {
	    /* none of the DDB files present */
	    errno = 0;
	    return(FAILURE);
	} 

	/* all DDB files present, & magicno's matched */
	return(TRUE);
}

/*
 *  int getmagicno(fp, magicno, timeval)
 *	FILE	*fp;
 *	char	*magicno;
 *	int	*timeval;
 *
 *	This function gets the "magic number", from the specified
 *	file, <fp>, and returns it in the buffer pointed by <magicno>.
 *
 *  Arguments:
 *	fp	Open file pointer to any one of the DDB files -
 *		DDB_TAB, DDB_SEC, DDB_DSFMAP or a temporary DDB file.	
 *	magicno	char buffer, in which magic number is returned.
 *	timeval	timestamp value
 *
 *  Returns:	int
 *	SUCCESS	if function returns successfuly
 *	FAILURE	if error during read
 */
int
getmagicno(fp, magictok,timeval)
FILE	*fp;
char	*magictok;
unsigned long *timeval;
{

	if (fscanf(fp,"%s %u\n",magictok,timeval))
		return(SUCCESS);
	return(FAILURE);
}
/*
 *  int setmagicno(fp)
 *	FILE	*fp;
 *
 *	This function writes the current "magic number" into the
 *	specified file <fp>, at the current offset. The current
 *	"magic number" is got from the static buffer <Magic_no[]>.
 *
 *	The static buffer <Magic_no[]>, can be set to a new value
 *	by using cr_magicno().
 *
 *  Arguments:
 *	fp	Open file pointer to any one of the DDB files -
 *		DDB_TAB, DDB_SEC, DDB_DSFMAP or a temporary DDB file.	
 *
 *  Returns:	int
 *	+ve value	if function returns successfuly
 *	0		if error during read
 */
int
setmagicno(fp)
FILE	*fp;
{

	return(fputs(Magic_no, fp));
}	
/*
 *  void cr_magicno()
 *	This function creates a new magic number in <Magic_no[]>,
 *	a static buffer. The current time, as returned by time(2),
 *	is used as part of new magic number.
 *	The magic number will be "MAGIC%NO value_returned_from_time"
 *
 *  Arguments:
 *
 *  Returns:	void
 */
void
cr_magicno()
{
	extern time_t	time();
	time_t		currt;
	char 		timestr[16];
	char		*magicptr;

	currt = time((time_t *)NULL);
	strcpy(Magic_no, MAGICTOK);

	/* There is  a space between MAGICTOK and time value */
	sprintf(timestr," %u\n",currt); 
	strcat(Magic_no, timestr);
}
