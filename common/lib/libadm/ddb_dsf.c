/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/ddb_dsf.c	1.2.14.3"
#ident	"$Header: $"

#include <stdio.h>	/* standard I/O definitions     */
#include <string.h>	/* string handling definitions  */
#include <ctype.h>	/* character types & macros     */
#include <errno.h>	/* error codes                  */
#include <stdlib.h>	/* storage alloc functions      */
#include <sys/types.h>	/* System data types            */
#include <sys/stat.h>	/* File status information      */
#include <sys/time.h>	/* Time definitions             */
#include <sys/mac.h>	/* MAC/SDH definitions          */
#include <devmgmt.h>	/* global devmgmt definitions   */
#include "devtab.h"	/* local devmgmt definitions    */

/*
 * Functions defined in ddb_dsf.c:
 *
 *	make_dsfent()	makes dsf_entry from input string
 *	get_dsfent()	returns dsf_entry with dsfs mapped to specified alias
 *	put_dsfent()	adds dsfs(in dsf_entry) and maps them to alias
 *	mod_dsfent()	modifies dsfs(in dsf_entry) and maps them to alias
 *	rem_dsfent()	removes dsfs mapped to specified alias from DDB_DSFMAP
 *	getdsfmap()	returns the alias & sec_alias mapped to dsf
 *	getnextdsf()	returns the next dsf mapped to specified alias
 *	make_devfiles()	makes dev_files array from dsf_entry
 *	conv_devfile()	builds DDB_DSFMAP record from input dev_file.
 *	free_devfiles()	frees memory allocated to dev_files
 *	freedsfent()	frees memory allocated dsf_entry and fields
 *	_insert_dsf()	inserts dsf in ascending order into dev_files array	
 *	get_dsfattr()	returns value of specified attr from dsf_entry.
 *	make_dsfrem()	makes dsf_entry marking specified attrs for removal
 */

#define GETDSFTYPE(dtype)	((strcmp(dtype,DDB_CDEVICE)==0)? CDEV:\
				((strcmp(dtype,DDB_BDEVICE)==0)? BDEV:\
				((strcmp(dtype,DDB_CDEVLIST)==0)? CDEVL:\
				((strcmp(dtype,DDB_BDEVLIST)==0)? BDEVL:0))))

static char	*Rem_dsfattr = "-";	/* marker for dsfattr to be removed */
void		free_devfiles();	/* frees memory for dev_files array*/

/*
 *  int make_dsfent(attr, value, field, dsfs)
 *	char        *attr;
 *	char        *value;
 *	int	    field;
 *	dsf_entry   *dsfs;
 *
 *  This function validates the specified <value> for <attr>, and then
 *  initializes the specified <field> in the dsf_entry, <dsfs>.
 *  If <value> is valid, it allocates memory for value string (malloc()).
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	value	- value of dsf attribute
 *	field	- field number in dsf_entry.
 *	dsfs	- specified field of structure dsf_entry,
 *		  initialized on return from function.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		if syntax error or invalid value
 *
 *  Notes:
 *    -	Given an attr=value string, the <field> number in dsf_entry
 *	structure is obtained by inviking getattrtype().
 */

int
make_dsfent(attr, value, field, devrec)
	char   *attr;
	char   *value;
	int	field;
	dev_record   *devrec;
{
	char	*dsfbuf;		/* memory for dsf entry's value  */
	char	*tabbuf;		/* memory for tab entry's value */
	int	err;			/* zero, if no error      */
	char	*next, *tmp;
	int	cnt;


	/* extract value(if defined) from within quotes */
	if (value == (char *)NULL) {
	    ddb_errmsg (SEV_ERROR, EX_USAGE, E_USAGE);
	    return(FAILURE);
	}

	/* allocate memory for value string */
	if (dsfbuf=(char *)malloc(strlen(value)+1)) {
		/* copy value into memory buffer just allocated */
	    	strcpy(dsfbuf, value);
	} else {
		ddb_errmsg (SEV_ERROR, EX_INTPRB, E_NOMEM);
		return(FAILURE);
	}

	/* initialize corresponding field in dsf_entry */
	switch(field) {
	    case (0):		/* attr = DDB_CDEVICE         */
		
		if (valid_path(value) && (getlistcnt(value) == 1)) {
		    devrec->dsf->cdevice = dsfbuf;

		    /* allocate memory for tab->cdevice */
	   	    if (tabbuf=(char *)malloc(strlen(value)+1)) {
			/* copy value into memory buffer just allocated */
	    		strcpy(tabbuf, value);
		    } else {
			ddb_errmsg (SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		    devrec->tab->cdevice = tabbuf;
		} else {
		    free(dsfbuf);
		    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_CDEVICE);
		    return(FAILURE);
		}
		break;

	    case (1):		/* attr = DDB_BDEVICE         */

		if (valid_path(value) && (getlistcnt(value) == 1)) {
		    devrec->dsf->bdevice = dsfbuf;
	   	    if (tabbuf=(char *)malloc(strlen(value)+1)) {
			/* copy value into memory buffer just allocated */
	    		strcpy(tabbuf, value);
		    } else {
			ddb_errmsg (SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		    devrec->tab->bdevice = tabbuf;
		} else {
		    free(dsfbuf);
		    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_BDEVICE);
		    return(FAILURE);
		}
		break;

	    case (2):		/* attr = DDB_CDEVLIST        */

		if (cnt=getlistcnt(value)) {
		    /* atleast 1 path defined in cdevlist     *
		     * validate if all are absolute pathnames */
		    next = value;
		    while ((cnt>0) && (next)) {
			if (!valid_path(next)) {
			    free(dsfbuf);
			    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_CDEVLIST);
			    return(FAILURE);
			}
			tmp = next;
			next = skpfield(tmp,",");
			cnt--;
		    }
		    /* validate first or last pathname */
		    if (!valid_path(tmp)) {
			free(dsfbuf);
			ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_CDEVLIST);
			return(FAILURE);
		    }
		    devrec->dsf->cdevlist = dsfbuf;

		} else {
		    free(dsfbuf);
		    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_CDEVLIST);
		    return(FAILURE);
		}
		break;

	    case (3):		/* attr = DDB_BDEVLIST        */

		if (cnt=getlistcnt(value)) {
		    /* atleast 1 path defined in cdevlist     *
		     * validate if all are absolute pathnames */
		    next = value;
		    while ((cnt>0) && (next)) {
			if (!valid_path(next)) {
			    free(dsfbuf);
			    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_BDEVLIST);
			    return(FAILURE);
			}
			tmp = next;
			next = skpfield(tmp,",");
			cnt--;
		    }
		    /* validate first or last pathname */
		    if (!valid_path(tmp)) {
			free(dsfbuf);
			ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_BDEVLIST);
			return(FAILURE);
		    }
		    devrec->dsf->bdevlist = dsfbuf;
		} else {
		    free(dsfbuf);
		    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_BDEVLIST);
		    return(FAILURE);
		}
		break;
	    default:
		return(FAILURE);
	}

	/* all's well, good attr value */
	return(SUCCESS);
}

/*
 *  dsf_entry *get_dsfent(alias)
 *	char        *alias;
 *
 *  This function returns the pathnames to dsfs mapped to a specified 
 *  <alias>, in the structure <dsf_entry>. It searches for the <alias>
 *  in the logical alias name field of each DDB_DSFMAP file entry.
 *
 *  Arguments:
 *	alias	- device alias name
 *
 *  Returns: sec_entry
 *	- Returns dsfs in <dsf_entry>. Memory for dsf_entry
 *	  and its field values is allocated within this function.
 *	- Returns NULL ptr on ERROR, and sets error code(ddb_errset()).
 *
 */
dsf_entry *
get_dsfent(alias)
	char        *alias;
{
	char		next_dsf[MAXDSFLEN]; /* next dsf maps to alias      */
	FILE		*fp;		/* file pointer                */
	dsf_entry	*dsfent;	/* dsf_entry                   */
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_DSFMAP   */
	int		dtype;		/* dsf attribute type          */
	int		clfirst, blfirst; /* first time flags          */
	int		err;		/* error flag                  */
	int		found;
	char		buf[80];	/* buf where file's first line 
					 * is  stored. It is not used */

	err = found = FALSE;

	/* open DDB_DSFMAP file for read only */
	if ((fp = fopen(DDB_DSFMAP, "r")) == (FILE *)NULL ) {
            /* error, cannot open DDB_DSFMAP file for read */
	    ddb_errset(errno);
	    return((dsf_entry *)NULL);
	}

	/* skip magic no */
	if (fgets(buf,80,fp) == (char *)NULL)  {
            /* error on reading DDB_DSFMAP file */
	    ddb_errset(errno);
	    fclose(fp);
	    return((dsf_entry *)NULL);
	}

	/* allocate memory for dsf_entry */
	if((dsfent=(dsf_entry *)malloc(sizeof(dsf_entry)))== NULL) {
		/* error ran out of memory */
		ddb_errset(errno);
		fclose(fp);
		return((dsf_entry *)NULL);
	}

	/* initialize fields of <dsfent> */
	INIT_DSFENTRY(dsfent);

	/* set first time flags to TRUE */
	clfirst = blfirst = TRUE;	

	/* get next dsf that maps to <alias> from DDB_DSFMAP */
	while(getnextdsf(fp,alias,DEV_ALIAS,next_dsf,&dtype)==SUCCESS) {

		/* initialize field of dsf_entry based on dtype */

		switch(dtype) {
		case (CDEV):

		    if (dsfent->cdevice=(char *)malloc(strlen(next_dsf)+1)) {
			strcpy(dsfent->cdevice,next_dsf);
		    } else {
			ddb_errset(errno);
			return((dsf_entry *)NULL);
		    }
		    break;

		case (BDEV):

		    if (dsfent->bdevice=(char *)malloc(strlen(next_dsf)+1)) {
			strcpy(dsfent->bdevice,next_dsf);
		    } else {
			ddb_errset(errno);
			return((dsf_entry *)NULL);
		    }
		    break;

		case (CDEVL):

		    /* is attribute found first time ? */
		    if (clfirst) {
			/* yes, then allocate memory for list */
			if (dsfent->cdevlist=malloc(MAXDSFLEN)) {
			    clfirst = FALSE;
			    /* copy next_dsf into dsfent field */
			    strcpy(dsfent->cdevlist, next_dsf);
			} else {
			    ddb_errset(errno);
			    return((dsf_entry *)NULL);
			}
		    } else {
			/* check if there is need for more memory */
			if ((int) strlen(dsfent->cdevlist) 
				+ (int) strlen(next_dsf) + 1 >= MAXDSFLEN) {
				dsfent->cdevlist= realloc(dsfent->cdevlist,
					strlen(dsfent->cdevlist)+
					strlen(next_dsf) + MAXDSFLEN);
				if (dsfent->cdevlist == (char *)NULL) {
			    		ddb_errset(errno);
			    		return((dsf_entry *)NULL);
				}
			}
			/* append dsfs to end separated by "," */
			strcat(dsfent->cdevlist, ",");
			strcat(dsfent->cdevlist, next_dsf);
		    }
		    break;	

		case (BDEVL):

		    /* is attribute found first time ? */
		    if (blfirst) {
			/* yes, then allocate memory for list */
			if (dsfent->bdevlist=malloc(MAXDSFLEN)) {
			    blfirst = FALSE;
			    /* copy next_dsf into dsfent field */
			    strcpy(dsfent->bdevlist, next_dsf);
			} else {
			    ddb_errset(errno);
			    return((dsf_entry *)NULL);
			}
		    } else {
			/* check if there is need for more memory */
			if ((int) strlen(dsfent->bdevlist) 
				+ (int) strlen(next_dsf) + 1 >= MAXDSFLEN) {
				dsfent->bdevlist= realloc(dsfent->bdevlist,
					strlen(dsfent->bdevlist)+
					strlen(next_dsf) + MAXDSFLEN);
				if (dsfent->bdevlist == (char *)NULL) {
			    		ddb_errset(errno);
			    		return((dsf_entry *)NULL);
				}
			}

			/* append dsfs to end separated by "," */
			strcat(dsfent->bdevlist, ",");
			strcat(dsfent->bdevlist, next_dsf);
		    }
		    break;	
		default:
		    ddb_errset(EFAULT);
		    return((dsf_entry *)NULL);
		}
	    }	/* end while */   

	/* check if error encountered */
	if (ddb_errget()) {
		fclose(fp);
		return((dsf_entry *)NULL);
	}
	
	/* SUCCESS, close files and return dsf_entry */
	fclose(fp);
	return(dsfent);
}

/*
 *  int put_dsfent(dsfent, alais, sec_alias)
 *	dsf_entry   *dsfent;
 *	char        *alias;
 *	char        *sec_alias;
 *
 *  This function creates a temporary file from the DDB_DSFMAP file,
 *  and adds the pathnames(dsfs) in dsf_entry, <dsfent>, to that file.
 *
 *  It maps each dsf in dsf_entry to the specified <alias> and <sec_alias>,
 *  and inserts the new pathnames(dsfs) into the temp DDB_DSFMAP file, 
 *  such that entries are ordered (ascending) by pathname. It also sets the
 *  new magic number on the temp DDB_DSFMAP file -- <Tmp_ddbfile[]>.
 *
 *  Arguments:
 *	dsfent	- pointer to dsf_entry
 *	alias	- dsfs mapped to alias 
 *	sec_alias- dsfs mapped to secure device alias 
 *
 *  Global Static:
 *	char *Tmp_ddbfile[]	- name of the temp DDB_DSFMAP file created.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		error
 *
 */

int
put_dsfent(dsfent, alias, sec_alias)
	dsf_entry   *dsfent;
	char        *alias;
	char        *sec_alias;
{
	FILE		*fp, *tmpfp;	/* file pointers               */
	char		*tmpddb;	/* temp DDB_DSFMAP filename    */
	dev_file	**devbuf;	/* array of ptrs(dev_file entries)*/
	int		devcnt, cnt;	/* number of dev_file entries  */
	char		*next;		/* ptr to next field           */
	char		*newrec;	/* record written out to temp file */
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recdsf;	/* dsf in curr record        */
	int		inserted;	/* inserted or not             */
	int		err;		/* error flag                  */
	int		cmp;		/* error flag                  */
	char		buf[80];	/* where file's first line 
					 * is  stored. It is not used */

	/* convert dsf_entry to a sorted list of dev_files */
	if (make_devfiles(dsfent, &devbuf, &devcnt) == FAILURE) {
	    return(FAILURE);
	}

	/* allocate space for new record to be added */
	if((newrec=(char *)
		malloc(MAXDSFLEN+strlen(DDB_CDEVLIST)+
					DDB_MAXALIAS+DDB_MAXALIAS))==NULL) {
	    /* ran out of memory */
	    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	}

	/* create temporary DDB_DSFMAP file, open for write */
	if ((tmpfp = opentmpddb(DDB_DSFMAP, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_DSFMAP file */
	    free(newrec);
	    return(FAILURE);
	}

	/* set new magic number   */
	setmagicno(tmpfp);	

	/* open DDB_DSFMAP file for read only */
	if ((fp = fopen(DDB_DSFMAP, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_DSFMAP file for read */
		fclose(tmpfp);
		free(newrec);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* skip magic no */
	if (fgets(buf,80,fp) == (char *)NULL) { 
	        /* error on reading DDB_DSFMAP file */
		fclose(tmpfp);
		fclose(fp);
		free(newrec);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	err = FALSE;
	cnt = 1;	/* cnt equals index into devbuf */

	/* copy records from DDB_DSFMAP to temp file.     *
	 * Insert new record in right place in temp file. */
	while ((!err) && (rec = read_ddbrec(fp))) {

		/* get dsf field of record */
		recdsf = getfield(rec,":",&next);

		while (cnt <= devcnt) {	/* inner while loop */

			/* compare dsf in devbuf[cnt] with record dsf */
			if ((cmp=strcmp(devbuf[cnt]->dsf,recdsf)) < 0) {
			    /* (new dsf) < (record dsf)     *
			     * insert new record right here */

			    /* build record(newrec) from input values */
			    conv_devfile(devbuf[cnt],alias,sec_alias,newrec);

			    /* write new record to Tmp_ddbdsf file */
			    if (write_ddbrec(tmpfp, newrec) < 0) {
				err = TRUE;
				free(rec);
				break;
			    }
			    cnt++;		/* index to next new dsf   */
			    *newrec = '\0';	/* reset newrec buffer     */
			} else if(cmp == 0) {
			    /* dsf already present in DDB_DSFMAP file */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_AEXIST,devbuf[cnt]->dsf);
			    err = TRUE;
			    break;
			} else break;		/* dsf > recdsf            */

		}	/* end inner while loop */
		if (err) 
			break;		/* exit outer loop also */

		/* replace ":" in record */
		*(--next) = ':';	

		/* copy old record to temp file */
		if (write_ddbrec(tmpfp, rec) < 0) {
		    err = TRUE;
		    break;
		}

		/* free mem alloc'ed for old rec */
		free(rec);	
	} /* end while */

	/* if EOF reached on DDB_DSFMAP file */
	while((!err)&&(cnt <= devcnt)) {
		/* add new records to end of temp DDB_DSFMAP file */
		/* build new record from input field values */
		conv_devfile(devbuf[cnt], alias, sec_alias, newrec);
		cnt++; 
		/* write new record into temp DDB_DSFMAP file */	    
		if (write_ddbrec(tmpfp, newrec) < 0)
			err = TRUE;
	}	/* end while */

	free(newrec);	/* free mem alloc'ed for newrec */

	if ((err)||(ddb_errget())) {
		/* internal error, delete temp DDB_DSFMAP file */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* close all files and return SUCCESS */
	fclose(fp);
	fclose(tmpfp);
	return(SUCCESS);
}

/*
 *  int mod_dsfent(mod, alias, s_alias, cmd)
 *	dsf_entry	*mod;
 *	char		*alias;
 *	char		*s_alias;
 *	int		cmd;
 *
 *  This function updates the existing values(pathname entries in DDB_DSFMAP)
 *  that are mapped to <alias>, with the new values defined in <mod>,
 *  based on the command option <cmd>.
 *
 *  If cmd = DEV_MOD:
 *  Then it modifies the pathnames mapped to <alias>, with the new values
 *  specified in <mod>.
 *
 *  If cmd = DEV_MODSEC:
 *  Then the <secdev> field of all existing and new dsfs are modified to the 
 *  char string specified in <s_alias>.
 *
 *  If cmd = DEV_REM:
 *  Then it removes the pathnames specified in <mod>.
 *
 *  Arguments:
 *	mod		dsf attributes to be modified
 *	alias		alias to which dsfs are mapped
 *	s_alias		secure device alias to be mapped to
 *	cmd		DEV_MOD/DEV_MODSEC/DEV_REM
 *
 *  Global Static:
 *	char *Tmp_ddbfile[]- name of the temp DDB_DSFMAP file created.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		error
 *
 */

int
mod_dsfent(mod, alias, s_alias, cmd)
	dsf_entry	*mod;
	char		*alias;
	char		*s_alias;
	int		cmd;
{
	FILE		*fp, *tmpfp;	/* file descriptors            */
	char		*tmpddb;	/* temp DDB_DSFMAP filename    */
	dsf_entry	*ddb,		/* dsf_entry from DDB_DSFMAP   */
			*get_dsfent();
	dev_file	**newdev,	/* new dev_files from mod   */
			**olddev;	/* old dev_files defined in DDB*/
	int		newcnt, oldcnt;	/* no. of dsfs in newdev & olddev  */
	int		ncnt, ocnt;	/* no. of dsfs in newdev & olddev  */
	char		*modrec;	/* record written out to temp file */
	char		*rec;		/* curr record in DDB_DSFMAP   */
	char		*recdsf;	/* dsf in curr record          */
	char		*dsf;		/* input dsf                   */
	char		*next;		/* ptr to next field           */
	int		cmp;		/* result of alias comparison  */
	int		err;		/* error flag                  */
	char 		buf[80];	/* where the first line of the 
					 * file will be read. Value is not used
					 */

	err = FALSE;

	/* get all dsfs mapped to alias from DDB_DSFMAP */
	if ((ddb=get_dsfent(alias)) == (dsf_entry *)NULL) {
	    /* error, could not access DDB_DSFMAP file */
	    return(FAILURE);
	}

	/* if cmd = DEV_MODSEC, then retain dsfs from DDB     *
	 * that are NOT being modified, to change secdev only */
	switch (cmd) {
	    case DEV_MODSEC:

		/* for dsf attrs NOT being modified */
		if ((mod->cdevice==(char *)NULL)&&(ddb->cdevice)) {
		   if (mod->cdevice=(char *)malloc(strlen(ddb->cdevice)+1)) {
			strcpy(mod->cdevice, ddb->cdevice);
		   } else
			err = TRUE;
		}
		if ((mod->bdevice==(char *)NULL)&&(ddb->bdevice)) {
		   if (mod->bdevice=(char *)malloc(strlen(ddb->bdevice)+1)) {
			strcpy(mod->bdevice, ddb->bdevice);
		   } else
			err = TRUE;
		}
		if ((mod->cdevlist==(char *)NULL)&&(ddb->cdevlist)) {
		   if (mod->cdevlist=(char *)malloc(strlen(ddb->cdevlist)+1)) {
			strcpy(mod->cdevlist, ddb->cdevlist);
		   } else
			err = TRUE;
		}
		if ((mod->bdevlist==(char *)NULL)&&(ddb->bdevlist)) {
		   if (mod->bdevlist=(char *)malloc(strlen(ddb->bdevlist)+1)) {
			strcpy(mod->bdevlist, ddb->bdevlist);
		   } else
			err = TRUE;
		}
		if (err) {
		    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		    return(FAILURE);
		}
		break;

	    case DEV_MOD:
	    case DEV_REM:

		/* remove dsf attrs in <ddb> that are not marked as  *
		 * being modified/removed in <mod> struct            */
		if (mod->cdevice == (char *)NULL) {
		   free(ddb->cdevice);	/* free memory */
		   ddb->cdevice = (char *)NULL; 
		}

		if (mod->bdevice == (char *)NULL) {
		   free(ddb->bdevice);	/* free memory */
		   ddb->bdevice = (char *)NULL; 
		}

		if (mod->cdevlist == (char *)NULL) {
		   free(ddb->cdevlist);	/* free memory */
		   ddb->cdevlist = (char *)NULL; 
		} else {
			if (cmd == DEV_REM) {
				/* is attribute being removed defined? */
				if (ddb->cdevlist == (char *)NULL) {
					/*attribute is not defined */
					ddb_errmsg(SEV_ERROR,EX_NOATTR,
						E_NOATTR,DDB_CDEVLIST,alias);
					err_report(Cmdname, ACT_CONT);
					ddb_errset(NOATTR);
				}
			}
		}

		if (mod->bdevlist == (char *)NULL) {
		   free(ddb->bdevlist);	/* free memory */
		   ddb->bdevlist = (char *)NULL; 
		} else {
			if (cmd == DEV_REM) {
				/* is attribute being removed defined? */
				if (ddb->bdevlist  == (char *)NULL) {
					ddb_errmsg(SEV_ERROR,EX_NOATTR,
						E_NOATTR,DDB_BDEVLIST,alias);
					err_report(Cmdname, ACT_CONT);
					ddb_errset(NOATTR);
				}
			} 
		}
		break;

	    default:
		return(FAILURE);
		break;
	}

	if (cmd == DEV_REM) {
		newcnt = 0;
		newdev = (dev_file **)NULL;
	} else {
		/*    make sorted dev_files to be added to DDB_DSFMAP file */
		if (make_devfiles(mod, &newdev, &newcnt) == FAILURE) {
		    /* internal error */
		    return(FAILURE);
	        }
	}

	/* make sorted list of dsfs to be removed from DDB_DSFMAP */
	if (make_devfiles(ddb, &olddev, &oldcnt) == FAILURE) {
		/* internal error */
		free_devfiles(newdev, newcnt);
		return(FAILURE);
	}

	/* allocate space for new record to be added */
	if((modrec=(char *)
		malloc(MAXDSFLEN+strlen(DDB_CDEVLIST)
					+DDB_MAXALIAS+DDB_MAXALIAS))==NULL) {
		/* ran out of memory */
		free_devfiles(olddev, oldcnt);
		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		return(FAILURE);
	}

	/* create temporary DDB_DSFMAP file, open for write */
	if ((tmpfp = opentmpddb(DDB_DSFMAP, &tmpddb)) == (FILE *)NULL) {
		/* error, cannot create temp DDB_DSFMAP file */
		free_devfiles(olddev, oldcnt);
		return(FAILURE);
	}

	setmagicno(tmpfp);	/* set new magic number   */

	/* open DDB_DSFMAP file for read only */
	if ((fp = fopen(DDB_DSFMAP, "r")) == (FILE *)NULL) {
		/* error, cannot open DDB_DSFMAP for read */
		fclose(tmpfp);
		free_devfiles(olddev, oldcnt);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* skip magic no */
	if  (fgets(buf, 80, fp) == (char *)NULL) {
		/* error on reading DDB_DSFMAP file */
		fclose(tmpfp);
		free_devfiles(olddev, oldcnt);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}


	ocnt = ncnt = 1;	/* reset index into olddev, newdev */

	/* copy records from DDB_DSFMAP to temp file.          *
	 * Insert modified record in right place in temp file. */
	while((!err) && (rec = read_ddbrec(fp))) { /* outer loop */

		/* extract pathname(dsf) from record */
		dsf = getfield(rec, ":", &next);

		/* check if old entry being modified/removed */
		if ((ocnt <= oldcnt)&&
			(strcmp(olddev[ocnt]->dsf,dsf) == 0)) {

			/* Skip to next dsf to be removed/modified */
			ocnt++;
		 	continue;
		}

		/* while there are pathnames to be added/modified */
		while(ncnt <= newcnt) {		/* inner loop */
			/* compare new dsf with record dsf */
			if ((cmp=strcmp(newdev[ncnt]->dsf, dsf)) == 0 ) {
			    /* new dsf already exists in DDB_DSFMAP */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_DSFEXIST, dsf);
			    err = TRUE;
			    break;
			} else if(cmp < 0) {
			    /* (new dsf) < (dsf in record)  *
			     * insert new dsf right here    */
			    /* build new record from input field values */
			    conv_devfile(newdev[ncnt],alias,s_alias,modrec);
			    
			    /* write new record to temp DDB_DSFMAP file */
			    if (write_ddbrec(tmpfp, modrec) < 0) {
				err = TRUE;
				break;
			    }
			} else {
			    /* (new dsf) > (dsf in record)  *
			     * Break out of inner loop.     */
			    break;
			}
			
			/* increment to next new/modified dsf */
			ncnt++;
		}	/* end inner while */

		if (err) {
			free(rec);
			break;	/* exit outer while loop also */
		}

		/* replace ":" in record */
		*(--next) = ':';	

		/* copy old record to temp file */
		if (write_ddbrec(tmpfp, rec) < 0) {
			  free(rec);
			  err = TRUE;
			  break;
		}

		/* free memory allocated by read_ddbrec() */
		free(rec);
	} /* end outer while */

	while((!err)&&(ncnt <= newcnt)) {

		/* add new records to end of temp DDB_DSFMAP file */
		/* build new record from input field values */
		conv_devfile(newdev[ncnt], alias, s_alias, modrec);

		/* write new record into temp DDB_DSFMAP file */
		if (write_ddbrec(tmpfp, modrec) < 0)
			err = TRUE;
		ncnt++;
	} /* end while */

	free(modrec);	/* free mem alloc'ed for modrec */

	if ((err)||(ddb_errget()>0 && ddb_errget()!= NOATTR)) {
		/* internal error, close all open files *
		 * Delete temp DDB_DSFMAP file.         */
		fclose(fp);
		fclose(tmpfp);
		rmtmpddb(tmpddb);
    		return(FAILURE);
	}
	    
	/* close all files and return */
	fclose(fp);
	fclose(tmpfp);

	if (ddb_errget() == NOATTR)
		return(NOATTR);

	return(SUCCESS);
}

/*
 *  int rem_dsfent(alias)
 *	char		*alias;
 *
 *  This function removes the pathnames(dsfs) mapped to the specified
 *  <alias>. It copies the remaining entries into the temp DDB_DSFMAP 
 *  file -- <Tmp_ddbfile[]>.
 *
 *  Arguments:
 *	alias	- dsfs mapped to specified alias are removed
 *
 *  Global Static:
 *	char *Tmp_ddbfile[]	- name of the temp DDB_SEC file created.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		error
 *
 */

int
rem_dsfent(alias)
	char		*alias;
{
	FILE		*fp, *tmpfp;	/* file descriptors            */
	char		*tmpddb;	/* temp DDB_DSFMAP filename    */
	dsf_entry	*remdsf;	/* dsf_entry from DDB_DSFMAP   */
	dev_file	**remdev;	/* array ptr(dev_files) to be removed */
	int		remcnt, rcnt;	/* no. of dev_files in remdev  */
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_DSFMAP   */
	char		*recalias;	/* alias in curr record        */
	char		*recdsf;	/* dsf in curr record          */
	int		cmp;		/* result of alias comparison  */
	int		err;		/* error flag                  */
	char		buf[80];	/* buffer were first line of file is 
					 * stored */

	err = FALSE;

	/* get all dsfs mapped to <alias> */
	if ((remdsf=get_dsfent(alias))==(dsf_entry *)NULL) {
	    return(FAILURE);
	}

	/* make sorted dev_files to be removed to DDB_DSFMAP file */
	if (make_devfiles(remdsf, &remdev, &remcnt) == FAILURE) {
	    /* internal error */
	    return(FAILURE);
	}

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

	rcnt = 1;

	/* copy records from DDB_DSFMAP to temp file.   *
	 * Remove those entries specified in <remdsf>   */
	while (rec = read_ddbrec(fp)) {
		if (rcnt <= remcnt) {

			/* get pathname(dsf) field of record */
			recdsf = getfield(rec,":",&next);

			/* compare alias with record alias */
			if ((cmp=strcmp(remdev[rcnt]->dsf,recdsf)) == 0) {
				/* (new alias) = (record alias) *
			     	 * remove this record           */
			    	free(rec);
			    	rcnt++;
			    	/* skip writing this record in Tmp_ddbsec */
			    	continue;
			}

			*(--next) = ':';	/* replace ":" in record */
		}

		/* copy old record to temp file */
	 	if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			free(rec);
			break;
	 	}

	 free(rec);	/* free memory - rec */
	} /* end while */

	/* if EOF reached on DDB_DSFMAP file */
	if ((err)||(ddb_errget())) {
		    /* internal error, close all open files *
		     * Delete temp DDB_DSFMAP file             */
		    fclose(fp);
		    fclose(tmpfp);
		    rmtmpddb(tmpddb);
	    	    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
	    	    return(FAILURE);
	}

	/* SUCCESS, close all files and return */
	fclose(fp);
	fclose(tmpfp);
	return(SUCCESS);
}

/*
 *  int getdsfmap(dsf, type, alias, sec_alias)
 *	char   *dsf;
 *	int  *type;
 *	char   *alias;
 *	char   *sec_alias;
 *
 *  Function searches for the specified pathname to device special
 *  file, <dsf>, and returns the type of dsf attribute <type>,
 *  and the <alias> and <sec_alias> it maps to, as defined in the
 *  DDB_DSFMAP file.
 *
 *  Arguments:
 *	dsf	- device special file to be searched in DDB_DSFMAP file.
 *	type	- dsf attribute type -- CDEV, BDEV, CDEVL, BDEVL.
 *	alias	- logical alias the dsf maps to.
 *	sec_alias - secure device alias the dsf maps to.
 *
 *  Returns: int
 *	SUCCESS	- if successful
 *	FAILURE	- if <dsf> not found, or error encountered, 
 *		  ddb error code set appropriately.
 *
 */

int
getdsfmap(dsf, type, alias, sec_alias)
	char   *dsf;
	int  *type;
	char   *alias;
	char   *sec_alias;
{
	char	*next_dsf, *next;	/* ptr to next dsf         */
	char	*al, *s_al;		/* ptr to alias, sec_alias */
	char	*dtype;			/* ptr to dsf attr type    */
	char	*rec;			/* curr record from DDB_DSFMAP */
	FILE	*fp;			/* FILE ptr to DDB_DSFMAP */
	register int found, cmp;	/* temp variables          */
	char 	buf[80];		/* buffer were first line of file is
					 * stored. This value is not used */

	if ((fp = fopen(DDB_DSFMAP, "r")) == (FILE *)NULL) {
	    /* error, cannot open DDB_DSFMAP */
	    ddb_errset(errno);
	    return(FAILURE);
	}

	/* skip magic number */
	if (fgets(buf, 80, fp) == (char *)NULL) {
	    /* error on reading DDB_DSFMAP */
	    ddb_errset(errno);
	    return(FAILURE);
	}

	found = FALSE;

	/* sequentially read DDB_DSFMAP records */
	/* read_ddb() allocates memory for the return value */
	while (rec = read_ddbrec(fp)) {

		/* extract dsf field from record */
		if ((next_dsf = getfield (rec,":", &next)) == (char *)NULL) {
			/* DDB_DSFMAP has problems */
			ddb_errset(EFAULT);		
			/* free memory allocated by read_ddbrec() */
			free(rec);
			break;
		}

		/* compare next_dsf with input dsf */
		if ((cmp=strcmp(dsf,next_dsf)) == 0) {

			/* found entry for dsf in DDB_DSFMAP */
			found = TRUE;

			/* extract dsf attr type */
			if (dtype = getfield(next,":", &next)) {
			    /* return dsf attr type */
			    *type = GETDSFTYPE(dtype);
			} else {
			    ddb_errset(EFAULT);	/* internal error */
			    free(rec);
			    break;
			}

			/* extract logical alias */
			if (al = getfield(next,":", &next)) {
			    /* copy alias to output buffer */
			    strcpy(alias, al);
			} else {
			    ddb_errset(EFAULT);	/* internal error */
			    free(rec);
			    break;
			}

			/* extract secure device alias,if defined */
			if (s_al = getfield(next,"\n", &next)) {
			    /* copy secure alias to output buffer */
			    strcpy(sec_alias, s_al);
			}
			free(rec);
			break;	/* break out of loop */

		    } else if (cmp < 0) {
			/* stop searching; dsf not found */
			free(rec);
			break;
		    }
		free(rec);		/* free memory alloc'd for rec */
	}	/* end while */

	/* dsf not defined in DDB_DSFMAP and no error was encountered */
	if (!found && !ddb_errget()) 
		ddb_errset(ENODEV);	

	fclose(fp);

	if (ddb_errget())
	    return(FAILURE);

	return(SUCCESS);
}

/*
 *  int getnextdsf(fp, alias, atype, dsf, dsftype)
 *	FILE	*fp;
 *	char	*alias;
 *	int	atype;
 *	char	*dsf;
 *	int	*dsftype;
 *
 *  Function searches for the specified <alias> and returns the
 *  the next pathname(<dsf>), that is mapped to the specified <alias>.
 *  The DDB_DSFMAP entry field that <device> is compared against,
 *  is based on the value of <dtype>.
 *  If <dtype> = DEV_ALIAS - <alias> compared against logical alias field
 *               DEV_SECDEV- <alias> compared against secure alias field 
 *
 *  When match is found it returns the next pathname(dsf), and the
 *  type of dsf attribute in <dsftype>.
 *
 *  Arguments:
 *	fp	- file pointer to an open DDB_DSFMAP file
 *	alias	- alias to be searched for in DDB_DSFMAP file
 *	atype	- alias type -- DEV_ALIAS, DEV_SECDEV
 *	dsf	- ptr to buffer in which the next dsf is returned
 *	dsftype	- dsf attribute type returned
 *
 *  Returns: int
 *	SUCCESS		if function returns successfully.
 *	FAILURE		if no more dsfs mapped to alias or
 *			error encountered (error code is set via ddb_errset())
 *
 */

int
getnextdsf(fp, alias, atype, dsf, dsftype)
	FILE	*fp;
	char	*alias;
	int	atype;
	char	*dsf;
	int	*dsftype;
{
	char	*next_dsf, *next;	/* ptr to next dsf         */
	char	*al, *s_al;		/* ptr to alias, sec_alias */
	char	*dtype;			/* ptr to dsf attr type    */
	char	*rec;			/* curr record from DDB_DSFMAP */

	next_dsf = (char *)NULL;
	*dsftype = (dsf_t)NULL;

	/* sequentially read DDB_DSFMAP records */
	while (rec = read_ddbrec(fp)) {

	    /* skip two fields in record -- dsf and dsf_type */
	    dtype = skpfield(rec,":");
	    next = skpfield(dtype,":");

	    /* extract logical alias field from record */
	    al = getfield(next,":",&next);

	    if (atype == DEV_ALIAS) {

		/* compare logical alias with input alias */
		if (strcmp(al,alias) == 0) {
		    /* found entry dsf in DDB_DSFMAP */

		    /* return pathname(dsf) & dsf attr type */
		    next_dsf = getfield(rec,":",&next);
		    dtype = getfield(dtype,":",&next);
		    *dsftype = GETDSFTYPE(dtype);

		    break;
		}
	    } else if (atype == DEV_SECDEV) {
		/* skip to secure device alias field in record */
		s_al = getfield(next, "\n", &next);

		/* compare secure alias with input alias */
		if (strcmp(s_al, alias) == 0) {
		    /* found entry in DDB_DSFMAP */

		    /* return pathname(dsf) & dsf attr type */
		    next_dsf = getfield(rec,":",&next);
		    dtype = getfield(dtype,":",&next);
		    *dsftype = GETDSFTYPE(dtype);

		    break;
		}
	    } else {
		/* invalid atype */
		ddb_errset(EINVAL);
		free(rec);
		break;
	    }
	    free(rec);		/* free memory alloc'd to rec */
	}	/* end while */

	if(next_dsf) {
	    /* copy next_dsf to output buffer */
	    strncpy(dsf, next_dsf, MAXDSFLEN);
	    free(rec);
	    return(SUCCESS);
	} 
	return(FAILURE);
}

/*
 *  int make_devfiles(dsfent, devbuf, cnt)
 *	dsf_entry	*dsfent;
 *	dev_file	***devbuf;
 *	int		*cnt;
 *
 *	This function converts the specified dsf_entry, <dsfent>,
 *	into a sorted list of items of type dev_file, and returns
 *	them in a malloc'ed buffer, <devbuf>. It also returns
 *	the number of dsfs (dev_file entries) in <cnt>.
 *	Each dev_file entry contains the pathname to device special file,
 *	and the type of attribute it is defined in --
 *	        CDEV, BDEV, CDEVL, BDEVL.
 *
 *	Memory for dev_file entries is allocated using malloc().
 *
 *  Arguments:
 *	dsfent	ptr to dsf_entry that contains dsf attrs
 *	devbuf	contains sorted dev_file entries, returned.
 *	cnt	number of dev_file entries returned in <devbuf>
 *
 *  Returns:
 *	SUCCESS   - if successful, a sorted ptr to dev_file entries returned.
 *	FAILURE   - if one of the dsf is not valid, or dsf is multiply
 *			defined.
 *
 */

int
make_devfiles(dsfent, devbuf, cnt)
	dsf_entry	*dsfent;
	dev_file	***devbuf;
	int		*cnt;
{
	char		*p, *next;	/* temp char pointers           */
	dev_file	*dev,		/* ptr to dev_file entry        */
			**devptr;	/* array of ptrs to dev_files   */
	char		*d, *dp;	/* temp buffer pointers         */
	int		item, total;
	int		i, clcnt, blcnt;

	/* get total number of pathnames defined in dsf_entry */
	clcnt = getlistcnt(dsfent->cdevlist);
	blcnt = getlistcnt(dsfent->bdevlist);
	total = ((dsfent->cdevice != (char *)NULL)?1:0)
		+ ((dsfent->bdevice != (char *)NULL)?1:0)
		+ clcnt + blcnt;
	if (total==0) {
	    *devbuf = (dev_file **)NULL;
	    *cnt = 0;
	}

	/* allocate memory for array of ptrs to dev_file entries *
	 * and array of dev_file entries themselves              */
	dp=(char *)malloc((total+1)*(sizeof(dev_file *) + sizeof(dev_file)));

	if (dp == (char *)NULL) {
	    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    ddb_errset(errno);
	    return(FAILURE);
	}

	/* set start of array of ptrs to dev_file entries */
	devptr = (dev_file **)dp;

	/* set start of array of dev_file entries */
	d = (char *)(dp+(total+1)*(sizeof(dev_file*)));
	dev = (dev_file *)d;

	/* init 1st dev_file entry to NULL values for sorting */
	item = 0;
	dev[item].dsf = "";
	dev[item].type = 0;

	/*
	 * start inserting dsfs into dev_file array
	 * in ascending order of pathnames to dsfs
	 */

	/* insert DDB_CDEVICE */
	if (dsfent->cdevice) {
		item++;
		/* init ptr to dev_file entry */
		devptr[item] = &dev[item];	

		/* init fields of dev_file entry */
		d = dev[item].dsf = dsfent->cdevice;

		dev[item].type = CDEV;
		/* insert entry into sorted list */
		if (_insert_dsf(item, devptr[item], devptr) < 0) {
		    free(dp);
		    ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, d, DDB_CDEVICE);
		    return(FAILURE);
		}
	}

	/* insert DDB_BDEVICE */
	if (dsfent->bdevice) {
		item++;
		/* init ptr to dev_file entry */
		devptr[item] = &dev[item];	

		/* init fields of dev_file entry */
		d = dev[item].dsf = dsfent->bdevice;

		dev[item].type = BDEV;
		/* insert entry into sorted list */
		if (_insert_dsf(item, devptr[item], devptr) < 0) {
		    free(dp);
		    ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, d, DDB_BDEVICE);
		    return(FAILURE);
		}
	}

	/* insert DDB_CDEVLIST items */
	if (clcnt) {
		item++;
		p = next = dsfent->cdevlist;	/* ptr to start of string */

		/* extract dsfs from list and *
		 * insert into dev_file array */
		for (i=1 ; i<clcnt ; i++,item++) {

		    devptr[item] = &dev[item];
		    /* extract next pathname */
		    d = dev[item].dsf = getfield(p,",", &next);
		    dev[item].type = CDEVL;

		    /* insert entry into sorted list */
		    if (_insert_dsf(item, devptr[item], devptr) < 0) {
			free(dp);
		        ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, d, DDB_CDEVLIST);
			return(FAILURE);
		    }
		    p = next;		/* bump ptr to next dsf */

		}
		/* insert last item in cdevlist */
		devptr[item] = &dev[item];

		/* get last or only pathname */
		d = dev[item].dsf = next;
		dev[item].type = CDEVL;

		/* insert entry into sorted list */
		if (_insert_dsf(item, devptr[item], devptr) < 0) {
			free(dp);
		        ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, d, DDB_CDEVLIST);
			return(FAILURE);
		}
	}

	/* insert DDB_BDEVLIST items */
	if (blcnt) {
		item++;
		p = next = dsfent->bdevlist;	/* ptr to start of string */

		/* extract dsfs from list and *
		 * insert into dev_file array */
		for (i=1 ; i<blcnt ; i++,item++) {

		    devptr[item] = &dev[item];

		    /* extract next pathname */
		    d = dev[item].dsf = getfield(p,",", &next);
		    dev[item].type = BDEVL;

		    /* insert entry into sorted list */
		    if (_insert_dsf(item, devptr[item], devptr) < 0) {
			free(dp);
		        ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, d, DDB_BDEVLIST);
			return(FAILURE);
		    }

		    p = next;		/* bump ptr to next dsf */
		}
		/* insert last item in bdevlist */
		devptr[item] = &dev[item];

		/* get last or only pathname */
		d = dev[item].dsf = next;
		dev[item].type = BDEVL;

		/* insert entry into sorted list */
		if (_insert_dsf(item, devptr[item], devptr) < 0) {
			free(dp);
		        ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, d, DDB_BDEVLIST);
			return(FAILURE);
		}
	}

	/* return pointer to array of sorted dev_file ptrs */
	*devbuf = devptr;
	*cnt = item;
	return(SUCCESS);
}

/*
 *  int conv_devfile(devf, alias, s_alias, recbuf)
 *	dev_file	*devf;
 *	char		*alias;
 *	char		*s_alias;
 *	char		*recbuf;
 *
 *  This function uses the input dev_file <devf>, <alias>, & <s_alias>,
 *  and creates a DDB_DSFMAP record in the specified buffer, <recbuf>.
 *
 *  Arguments:
 *	devf		contains pathname(dsf) and dsf attr type
 *	alias		alias name
 *	s_alias		secure device alias name
 *	recbuf		buffer ptr to DDB_DSFMAP record.
 *
 *  Returns: int
 */

int
conv_devfile(devf, alias, s_alias, recbuf)
	dev_file	*devf;
	char		*alias;
	char		*s_alias;
	char		*recbuf;
{
	*recbuf = '\0';

	strcpy(recbuf, devf->dsf);
	strcat(recbuf,":");
	switch(devf->type) {
	    case(CDEV):
		strcat(recbuf, DDB_CDEVICE);
		break;
	    case(BDEV):
		strcat(recbuf, DDB_BDEVICE);
		break;
	    case(CDEVL):
		strcat(recbuf, DDB_CDEVLIST);
		break;
	    case(BDEVL):
		strcat(recbuf, DDB_BDEVLIST);
		break;
	    default:
		break;
	}
	strcat(recbuf,":");
	strcat(recbuf, alias);
	strcat(recbuf,":");
	strcat(recbuf, s_alias);
	strcat(recbuf,"\n");
}

/*
 *  void free_devfiles(devbuf)
 *	char	*devbuf;
 *
 *	This function free's all the memory allocated for 
 *      the array of dev_file pointers, and the array of dev_file
 *      entries they point to. This memory is allocated in the
 *      function make_devfiles(), as one single chunk accomodating
 *	both arrays.
 *
 *  Arguments:
 *	devbuf	contains array of ptrs to dev_files and array of dev_files
 *
 *  Returns: int
 *
 */

void
free_devfiles(devbuf)
	char	*devbuf;
{
	free(devbuf);
}

/*
 *  void freedsfent(dsfent)
 *	dsf_entry	*dsfent;
 *
 *	This function free's the memory allocated for the specified
 *	dsf_entry structure, <dsfent>, and its individual field values.
 *      This memory is allocated in the function make_dsfent().
 *
 *  Arguments:
 *	dsfent		pointer to the dsf_entry structure.
 *
 *  Returns: int
 *
 */

void
freedsfent (dsf_entry *dsfent)
{
	/* free memory for fields of dsf_entry structure */
	free(dsfent->cdevice);
	free(dsfent->bdevice);
	free(dsfent->cdevlist);
	free(dsfent->bdevlist);
	/* free memory allocated for struct dsf_entry */
	free(dsfent);
}

/*
 *  int _insert_dsf(n, dev, devp)
 *	int        n;
 *	dev_file  *dev;
 *	dev_file  **devp;
 *
 *	This function inserts the item number, <n>, into an array of
 *	dev_file pointers, <devp>, based on comparing the dsf field
 *	of the specified entry, <dev>, with the rest of the sorted entries.
 *	It uses the standard Insertion Sort algorithm.
 *
 *  Arguments:
 *	n	item number being inserted
 *	dev	dev_file entry to be inserted
 *	devp	array of pointers to dev_file entries
 *
 *  Returns: int 
 *	SUCCESS	if successful
 *	FAILURE	if there are duplicate uids
 *
 */

int
_insert_dsf(int n, dev_file *dev, dev_file **devp)
{
	dev_file	*key;		/* entry to be inserted */
	int		i, cmp;

	key = dev;
	i = n-1;

	while (i > 0) {
	    /* compare key->dsf with each pathname(dsf) */
	    if ((cmp=strcmp(devp[i]->dsf, key->dsf)) > 0) {
		/* if array element > key, switch pointers to items */
		devp[i+1] = devp[i];
		i = i-1;
	    } else if (cmp == 0) {
		/* if array element = key, error, duplicate pathname(dsf) */
		ddb_errset(EEXIST);
		return(FAILURE);
	    } else {
		/* correct position of key found, break out */
		break;
	    }
	}
	/* insert key */
	devp[i+1] = key;
	return(SUCCESS);
}

/*
 *  char *get_dsfattr(attr, field, devrec)
 *	char        *attr;
 *	int         field;
 *	dev_record   *devrec;
 *
 *  This function returns the value of the specified <attr> from
 *  dsf_entry, <dsf>.
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	field	- field number in dsf_entry.
 *	dsf	- specified field of structure dsf_entry,
 *		  is returned.
 *
 *  Returns: char *
 *	Value of attribute field is returned.
 *
 */
char *
get_dsfattr(attr, field, devrec)
	char        *attr;
	int         field;
	dev_record   *devrec;
{
	char		*value;

	value = (char *)NULL;

	/* get corresponding field of dsf_entry */
	switch (field) {
		case (0):	/* attr = cdevice */

			/*
		 	 * So that 4.1 functions work with 4.0 format files
		 	 * use the value of cdevice found in device.tab
		  	 */
			if (devrec->tab->cdevice)
				value = devrec->tab->cdevice;
			break;

	    	case (1):	/* attr = bdevice */

			/*
		 	 * So that 4.1 functions work with 4.0 format files
		 	 * use the value of cdevice found in device.tab
		 	 */
			if (value = devrec->tab->bdevice)
				value = devrec->tab->bdevice;
			break;

	    	case (2):	/* attr = cdevlist  */

			if(devrec->dsf->cdevlist)
				value = devrec->dsf->cdevlist;
			break;

		case (3):	/* attr = bdevlist */

			if (devrec->dsf->bdevlist)
				value = devrec->dsf->bdevlist;
			break;

	    	default:
			break;
	}
	return (value);
}

/*
 *  int make_dsfrem(attr, field, dsfs)
 *	char        *attr;
 *	int	    field;
 *	dsf_entry   *dsfs;
 *
 *  This function marks the specified <attr> in the dsf_entry,
 *  <dsfs>, for removal from the corresponding entry in DDB_DSFMAP file.
 *  The value of specified <attr> is initialized to "-" (Rem_dsfattr).
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	field	- field number in dsf_entry.
 *	dsfs	- specified field of structure dsf_entry,
 *		  initialized on return from function.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		if syntax error or invalid value
 *
 */

int
make_dsfrem(attr, field, devrec)
	char   *attr;
	int	field;
	dev_record   *devrec;
{

	/* initialize corresponding field in dsf_entry */
	switch(field) {
	    case (0):		/* attr = DDB_CDEVICE         */
		devrec->dsf->cdevice = Rem_dsfattr;
		devrec->tab->cdevice = Rem_dsfattr;
		break;
	    case (1):		/* attr = DDB_BDEVICE         */
		devrec->dsf->bdevice = Rem_dsfattr;
		devrec->tab->bdevice = Rem_dsfattr;
		break;
	    case (2):		/* attr = DDB_CDEVLIST        */
		devrec->dsf->cdevlist = Rem_dsfattr;
		break;
	    case (3):		/* attr = DDB_BDEVLIST        */
		devrec->dsf->bdevlist = Rem_dsfattr;
		break;
	    default:
		return(FAILURE);
	}
	/* all's well */
	return(SUCCESS);
}
