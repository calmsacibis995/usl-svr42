/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/ddb_sec.c	1.1.13.2"
#ident	"$Header: ddb_sec.c 1.4 91/06/25 $"

#include <stdio.h>	/* standard I/O definitions     */
#include <string.h>	/* string handling definitions  */
#include <ctype.h>	/* character types & macros     */
#include <errno.h>	/* error codes                  */
#include <stdlib.h>	/* storage alloc functions      */
#include <sys/types.h>	/* System data types            */
#include <sys/stat.h>	/* File status information      */
#include <sys/time.h>	/* Time definitions             */
#include <mac.h>	/* MAC/SDH definitions          */
#include <devmgmt.h>	/* global devmgmt definitions   */
#include "devtab.h"	/* local devmgmt definitions    */

/*
 * E X T E R N A L  R E F E R E N C E S
 */
/*
 * Functions defined in ddb_sec.c:
 *
 *	make_secent()	makes sec_entry from input string
 *	freesecent()	frees memory allocated for field of sec_entry 
 *	get_secent()	returns the secure device entry for specified alias
 *	put_secent()	adds sec_entry to the DDB_SEC file
 *	mod_secent()	modifies entry in DDB_SEC file using sec_entry
 *	rem_secent()	removes entry in DDB_SEC file
 *	get_secattr()	returns value of specified attr from sec_entry.
 *	getnextsec()	returns the next secure device entry from DDB_SEC file
 */

/*
 * L O C A L  D E F I N I T I O N S
 *
 * Static functions defined in ddb_sec.c:
 *
 *	conv_secrec()	converts sec record (char *) into sec_entry
 *	conv_secent()	converts sec_entry into sec record (char *) 
 */
static char	*Rem_secattr = "-";	/* marker for secattr to be removed */


/*
 *  int make_secent(attr, value, field, sec)
 *	char        *attr;
 *	char        *value;
 *	int         field;
 *	sec_entry   *sec;
 *
 *  This function validates the specified <value> for <attr>, and then
 *  initializes the specified <field> in the sec_entry, <sec>.
 *  If <value> is valid, it allocates memory for value string (malloc()).
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	value	- value string defined for the attribute.
 *	field	- field number in sec_entry.
 *	sec	- specified field of structure sec_entry,
 *		  initialized on return from function.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		if syntax error or invalid value
 *
 *  Notes:
 *    -	Given an attr=value string, the <field> number in sec_entry
 *	structure is obtained by inviking getattrtype().
 */
int
make_secent(attr, value, field, sec)
	char        *attr;
	char        *value;
	int         field;
	sec_entry   *sec;
{
	char		*user, *group, *other, 
			*perm, *users;
	level_t		level;
	uid_t		uid;
	gid_t		gid;
	level_t		dev_hi, dev_lo;	

	/* check if value and field number are defined  */
	if ((value) && (field)) {
	    /* initialize corresponding field in sec_entry to value */
	    switch (field) {
	    case (1):			/* attr = RANGE               */
		/* parse input device range */
		if (parse_range(value, &dev_hi, &dev_lo)==SUCCESS) {
		    /* check if hilevel dominate lolevel ONLY if MAC running*/
		    if (mac_running() && (lvldom(&dev_hi, &dev_lo) == 0)) {
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_LVLDOM, DDB_RANGE);
			return(FAILURE);
		    } 
		    /* valid device range, convert to char string */
		    if (sec->range=(char *)malloc(MAXLVLSZ)) {
			sprintf(sec->range,"%d-%d",dev_hi,dev_lo);
		    } else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		} else {
		    /* error, invalid device range */	
		    return(FAILURE);
		}
		break;
	    case (2):			/* attr = STATE               */
		/* validate device state */
		if ((strcmp(value, DDB_PUBLIC)==0)||
		    (strcmp(value, DDB_PRIVATE)==0)||
		    (strcmp(value, DDB_PUB_PRIV)==0)) {
		    /* valid device state, initialize field */
		    if (sec->state=(char *)malloc(strlen(value)+1)) {
			strcpy(sec->state,value);
		    } else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		} else {
		    /* sec->state does not contain a valid value */
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL, DDB_STATE);
		    return(FAILURE);
		}
		break;
	    case (3):			/* attr = MODE                */
		/* validate device mode */
		if ((strcmp(value, DDB_STATIC)==0)||
		    (strcmp(value, DDB_DYNAMIC)==0)) {
		    /* valid device mode, initialize field */
		    if (sec->mode=(char *)malloc(strlen(value)+1)) {
			strcpy(sec->mode,value);
		    } else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		} else {
		    /* sec->mode does not contain a valid value */
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL, DDB_MODE);
		    return(FAILURE);
		}
		break;
	    case (4):			/* attr = DDB_STARTUP             */
		/* validate device startup */
		if ((strcmp(value,"y")==0)||
			(strcmp(value,"yes")==0)|| 
			(strcmp(value,"no")==0)|| 
			(strcmp(value,"n")==0)) {
		    /* valid device startup, initialize field */
		    if (sec->startup=(char *)malloc(2)) {
			strncpy(sec->startup,value,1);	/* copy 1st char   */
			*(sec->startup+1) = '\0';	/* NULL terminater */
		    } else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		} else {
		    /* sec->startup invalid */
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL, DDB_STARTUP);
		    return(FAILURE);
		}
		break;
	    case (5):			/* attr = DDB_ST_LEVEL       */
		/* parse input device startup_level */
		if (lvlin(value, &level)==SUCCESS) {
		    /* valid startup_level, initialize field */
		    if (sec->st_level=(char *)malloc(MAXLVLSZ)) {
			sprintf(sec->st_level,"%d",level);
		    } else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		} else {
		    switch(errno) {
		    case(EINVAL):
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_INLVL, DDB_ST_LEVEL, value);
			break;
		    default:
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			break;
		    }
		    return(FAILURE);
		}
		break;
	    case (6):			/* attr = STARTUP_OWNER       */
		if (user=getfield(value,">", &perm)) {
		    /* parse input device startup_owner */
		    if (parse_uid(user, &uid)==SUCCESS) {
			if (valid_stperm(perm)) {
			    /* valid startup_owner, initialize field */
			    if (sec->st_owner=(char *)malloc(MAXIDSZ+5)) {
				sprintf(sec->st_owner,"%d>%s", uid, perm);
		    	    } else {
				/* ran out of memory */
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
				return(FAILURE);
			    }
			} else {
			    /* invalid startup permissions */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INPRM, DDB_ST_OWNER, perm);
			    return(FAILURE);
			}
		    } else {
			/* invalid user/uid */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_INUID, DDB_ST_OWNER, user);
			return(FAILURE);
		    }
		} else {
		    /* error, invalid delimiter in startup_owner */	
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INDLM, DDB_ST_OWNER, value);
		    return(FAILURE);
		}
		break;
	    case (7):			/* attr = DDB_ST_GROUP       */
		if (group=getfield(value,">", &perm)) {
		    /* parse input device startup_group */
		    if (parse_gid(group, &gid)==SUCCESS) {
			if (valid_stperm(perm)) {
			    /* valid startup_group, initialize field */
			    if (sec->st_group=(char *)malloc(MAXIDSZ+5)) {
				sprintf(sec->st_group,"%d>%s", gid, perm);
		    	    } else {
				/* ran out of memory */
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
				return(FAILURE);
			    }
			} else {
			    /* invalid startup permissions */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INPRM, DDB_ST_GROUP, perm);
			    return(FAILURE);
			}
		    } else {
			/* invalid group/gid */
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_INGID, DDB_ST_GROUP, group);
			return(FAILURE);
		    }
		} else {
		    /* error, invalid delimiter in startup_group */	
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INDLM, DDB_ST_GROUP, value);
		    return(FAILURE);
		}
		break;
	    case (8):			/* attr = DDB_ST_OTHER       */
		if (other=getfield(value,">", &perm)) {
		    /* parse input device startup_other */
		    if (*other == NULL) {
			if (valid_stperm(perm)) {
			    /* valid startup_other, initialize field */
			    if (sec->st_other=(char *)malloc(5)) {
				sprintf(sec->st_other,">%s", perm);
		    	    } else {
				/* ran out of memory */
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
				return(FAILURE);
			    }
			} else {
			    /* invalid startup permissions */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INPRM, DDB_ST_OTHER, perm);
			    return(FAILURE);
			}
		    } else {
			/* invalid other value */
		        ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL, DDB_ST_OTHER);
			return(FAILURE);
		    }
		} else {
		    /* error, invalid delimiter in startup_other */	
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INDLM, DDB_ST_OTHER, value);
		    return(FAILURE);
		}
		break;
	    case (9):			/* attr = DDB_UAL_ENABLE          */
		/* validate device ual_enable */
		if ((strcmp(value,"y")==0)||
			(strcmp(value,"yes")==0)||
			(strcmp(value,"n")==0)||
			(strcmp(value,"no")==0)) {
		    /* valid device ual_enable, initialize field */
		    if (sec->ual_enable=(char *)malloc(2)) {
			strncpy(sec->ual_enable,value,1); /* copy 1st char   */
			*(sec->ual_enable+1) = '\0';	/* NULL terminater */
		    } else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		    }
		} else {
		    /* sec->ual_enable invalid */
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL, DDB_UAL_ENABLE);
		    return(FAILURE);
		}
		break;
	    case (10):			/* attr = DDB_USERS               */
		/* parse input device users(UAL) */
		if (parse_users(value, &users)==SUCCESS) {
		    /* initialize UAL in sec_entry */
		    sec->users = users;
		} else {
		    /* invalid users(UAL) */
		    return(FAILURE);
		}
		break;
	    case (11):			/* attr = OTHER               */
		if (other=getfield(value,">", &perm)) {
		    /* parse input device UAL other */
		    if (*other == NULL) {
			if (valid_perm(perm)) {
			    /* valid UAL other, initialize field */
			    if (sec->other=(char *)malloc(3)) {
				sprintf(sec->other,">%s", perm);
		    	    } else {
				/* ran out of memory */
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
				return(FAILURE);
			    }
			} else {
			    /* invalid UAL other permissions */
			    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INPRM, DDB_OTHER, perm);
			    return(FAILURE);
			}
		    } else {
			/* invalid other value */
		        ddb_errmsg(SEV_ERROR, EX_ERROR, E_INVAL, DDB_OTHER);
			return(FAILURE);
		    }
		} else {
		    /* error, invalid delimiter in startup_other */	
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INDLM, DDB_OTHER, sec->other);
		    return(FAILURE);
		}
		break;
	    default:
		return(FAILURE);
	    }
	} else {
	    /* syntax error */
	    ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
	}
	return (SUCCESS);
}

/*
 *  sec_entry get_secent(alias)
 *	char        *alias;
 *
 *  This function returns the security attributes for the specified <alias>,
 *  in the structure <sec_entry>.
 *
 *  Arguments:
 *	alias	- secure device alias name
 *
 *  Returns: sec_entry
 *	- Returns security attrs in <sec_entry>. Memory for sec_entry
 *	  is allocated within this function.
 *	- Returns NULL ptr on ERROR, and sets error code(ddb_errset()).
 *
 */

sec_entry *
get_secent(alias)
	char		*alias;		/* secure device alias */
{
	FILE		*fp;		/* file pointer                */
	sec_entry	*sec,		/* secure device entry         */
			*conv_secrec();
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recalias;	/* alias in curr record        */
	int		cmp;		/* result of alias comparison  */
	char		buf[80];	/* Where file's first line is stored.
					 * The value is not used */

	sec = (sec_entry *)NULL;

	/* open DDB_SEC file for read only */
	if ((fp = fopen(DDB_SEC, "r")) == (FILE *)NULL) {
            /* error, cannot open DDB_SEC file for read */
	    ddb_errset(errno);
	    return(NULL);
	}

	/* skip magic no */
	if (fgets(buf,80,fp) == (char *)NULL) {
            /* error reading DDB_SEC */
	    ddb_errset(errno);
	    return(NULL);
	}

	/* read records from DDB_SEC */
	/* read_ddbrec() allocates memory for rec */
	while (rec = read_ddbrec(fp)) {

		/* get alias field of record */
		if ((recalias = getfield(rec,":",&next)) == (char *)NULL) {
			/* File DDB_SEC has problems*/
			ddb_errset(EFAULT);         
			free(rec);
			break;
		}

		/* compare alias with record alias */
		if ((cmp=strcmp(alias,recalias)) == 0) {
		    	/* (alias) = (record alias) */

		    	/* replace ':' in record */
		    	*(--next) = ':';	

		    	/* convert ddb record to sec_entry */
		    	if ((sec = conv_secrec(rec)) == (sec_entry *)NULL) {
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		    	} 
			free(rec);
			break;

		} else if (cmp < 0) {
		    	/* alias < recalias; alias not found.*
		     	* Stop searching; DDB_SEC contains  *
		     	* entries ordered by alias name.    */
			free(rec);
			break;
		}
		free(rec);	/* free mem allocated for rec */
	} /* end while */

	fclose(fp);
	return(sec);
}

/*
 *  void  freesecent(sec)
 *	sec_entry       *sec;
 *
 *  This function free's the memory allocated for the input sec_entry, 
 *  <sec>, and its fields.
 *
 *  Arguments:
 *	sec   	- structure of type sec_entry
 *
 *  Returns: void
 *
 */
void
freesecent(sec)
sec_entry	*sec;
{
	/* free each sec_entry field value */ 
	free (sec->alias);
	free (sec->range);
	free (sec->state);
	free (sec->mode);
	free (sec->startup);
	free (sec->st_level);
	free (sec->st_owner);
	free (sec->st_group);
	free (sec->st_other);
	free (sec->ual_enable);
	free (sec->users);
	free (sec->other);
	/* free memory allocated to struct sec_entry */
	free (sec);
}

/*
 *  int put_secent(sec)
 *	sec_entry   *sec;
 *
 *  This function creates a temporary file from the DDB_SEC file,
 *  and adds the new sec_entry <sec>, to that file.
 *
 *  It inserts the new entry <sec> into the temp DDB_SEC file, such that
 *  entries are ordered (ascending) by alias name. It also sets the
 *  new magic number on the temp DDB_SEC file -- <Tmp_ddbfile[]>.
 *
 *  Arguments:
 *	sec	- pointer to sec_entry
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
put_secent(sec)
	sec_entry	*sec;		/* ptr to sec_entry            */
{
	FILE		*fp, *tmpfp;	/* file pointers               */
	char		*tmpddb;	/* temp DDB_SEC filename       */
	char		*next;		/* ptr to next field           */
	char		*newrec,	/* record written out to temp file */
			*conv_secent();
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recalias;	/* alias in curr record        */
	int		inserted;	/* inserted or not             */
	int		err;		/* error flag                  */
	char		buf[80];	/* Where file's first line is stored.
					 * The value is not used.	*/

	/* convert sec_entry to record (char *) */
	if ((newrec = conv_secent(sec)) == NULL) {
	    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    return(FAILURE);
	}
	err = inserted = FALSE;

	/* create temporary DDB_SEC file, open for write */
	if ( (tmpfp = opentmpddb(DDB_SEC, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_SEC file */
	    return(FAILURE);
	}

	/* set new magic number   */
	setmagicno(tmpfp);	

	/* open DDB_SEC file for read only */
	if ((fp = fopen(DDB_SEC, "r"))  == (FILE *)NULL) {
	        /* error, cannot open DDB_SEC file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* skip magic no */
	if (fgets(buf, 80, fp) == (char *)NULL) {
	        /* error reading DDB_SEC file */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* copy records from DDB_SEC to temp DDB_SEC file *
	 * Insert new record in right place in temp file. */
	while (rec = read_ddbrec(fp)) {
		    if (!inserted) {
			/* get alias field of record */
			recalias = getfield(rec,":",&next);

			/* compare alias with record alias */
			if (strcmp(sec->alias,recalias) < 0) {
			    /* (new alias) < (record alias) *
			     * insert new record right here */
			    if (write_ddbrec(tmpfp, newrec) < 0) {
				err = TRUE;
				break;
			    }
			    free(newrec);   /* free mem alloc'ed for newrec */
			    inserted = TRUE;
			}
			*(--next) = ':';	/* replace ":" in record */
		    }
		    /* copy old record to temp file */
		    if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			break;
		    }
		    free(rec);	/* free mem alloc'ed for old rec */
	} /* end while */

	/* if EOF reached on DDB_SEC file */
	if ((!inserted)&&(!err)) {
		    /* add new record to end of temp DDB_SEC file */
		    if (write_ddbrec(tmpfp, newrec) < 0)
			err = TRUE;
		    free(newrec);	/* free mem alloc'ed for newrec */
	}

	if ((err)||(ddb_errget())) {
		    /* internal error, delete temp DDB_SEC file */
		    fclose(tmpfp);
		    rmtmpddb(tmpddb);
	    	    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
	    	    return(FAILURE);
	}

	/* close all files and return SUCCESS */
	fclose(fp);
	fclose(tmpfp);
	return(SUCCESS);
}

/*
 *  int mod_secent(sec, cmd)
 *	sec_entry   *sec;
 *	int         cmd;
 *
 *  This function modifies the sec_entry <sec>, defined in DDB_SEC file,
 *  and puts the updated entry in the temp DDB_SEC file -- <Tmp_ddbfile[]>.
 *  If <cmd> = DEV_ATTRMOD, then it modifies the non-NULL fields(attr-values)
 *			in the sec_entry in DDB_SEC file.
 *  If <cmd> = DEV_ATTRREM, then it removes the non-NULL fields (attr-values)
 *			from the sec_entry in DDB_SEC file.
 *
 *  Arguments:
 *	sec	- pointer to sec_entry
 *	cmd	- DEV_ATTRMOD or DEV_ATTRREM
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
mod_secent(sec, cmd)
sec_entry	*sec; 
int		cmd;
{
	FILE		*fp, *tmpfp;	/* file descriptors            */
	char		*tmpddb;	/* temp DDB_SEC filename       */
	sec_entry	*ddbsec,	/* sec_entry from DDB_SEC      */
			*conv_secrec();
	char		*next;		/* ptr to next field           */
	char		*modrec,	/* record written out to temp file */
			*newrec,
			*conv_secent();
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recalias;	/* alias in curr record        */
	int		modified;	/* modified or not             */
	int		cmp;		/* result of alias comparison  */
	int		err;		/* error flag                  */
	char 		buf[80];	/* Where file's first line is stored.
					 * Value is not used.		*/

	err = modified = FALSE;

	/* create temporary DDB_SEC file, open for write */
	if ( (tmpfp = opentmpddb(DDB_SEC, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_SEC file */
	    return(FAILURE);
	}

	/* set new magic number   */
	setmagicno(tmpfp);	

	/* open DDB_SEC file for read only */
	if ( (fp = fopen(DDB_SEC, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_SEC file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	 /* skip magic no */
	if (fgets(buf,80,fp) == (char *)NULL) {
	        /* error, cannot open DDB_SEC file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

 	/* copy records from DDB_SEC to temp DDB_SEC file.     *
	 * Insert modified record in right place in temp file. */
	while (rec = read_ddbrec(fp)) {

		    if (!modified) {

			/* get alias field of record */
			recalias = getfield(rec,":",&next);

			/* compare alias with record alias */
			if ((cmp=strcmp(sec->alias,recalias)) == 0) {

			    /* (new alias) = (record alias) *
			     * modify this record           */
			    *(--next) = ':';	/* replace ':' in record */

			    /* convert ddb record to sec_entry */
			    if ((ddbsec = conv_secrec(rec)) == NULL) {
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
				return(FAILURE);
			    }

			    /* For cmd= DEV_MOD:                             *
			     * If input <sec> field not NULL, then copy      *
			     * new field value from <sec> to <ddbsec>.       *
			     * For cmd= DEV_REM:                             *
			     * If input <sec> field not NULL, then remove    *
			     * existing field value from <ddbtab>.           */

			    if (sec->range) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->range);
				    ddbsec->range = sec->range;
				    sec->range = (char *)NULL;
				} else {
					if (ddbsec->range) {
				    		free(ddbsec->range);
				    		ddbsec->range = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR, DDB_RANGE,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->state) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->state);
				    ddbsec->state = sec->state;
				    sec->state = (char *)NULL;
				} else {
					if(ddbsec->state) {
				    		free(ddbsec->state);
				    		ddbsec->state = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_STATE,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->mode) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->mode);
				    ddbsec->mode = sec->mode;
				    sec->mode = (char *)NULL;
				} else {
					if (ddbsec->mode) {
				    		free(ddbsec->mode);
				    		ddbsec->mode = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR, DDB_MODE,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->startup) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->startup);
				    ddbsec->startup = sec->startup;
				    sec->startup = (char *)NULL;
				} else {
					if (ddbsec->startup) {
				    		free(ddbsec->startup);
				    		ddbsec->startup = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_STARTUP,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->st_level) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->st_level);
				    ddbsec->st_level = sec->st_level;
				    sec->st_level = (char *)NULL;
				} else {
					if (ddbsec->st_level) {
				    		free(ddbsec->st_level);
				    		ddbsec->st_level = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_ST_LEVEL,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}

				}
			    }

			    if (sec->st_owner) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->st_owner);
				    ddbsec->st_owner = sec->st_owner;
				    sec->st_owner = (char *)NULL;
				} else {
					if (ddbsec->st_owner) {
				    		free(ddbsec->st_owner);
				    		ddbsec->st_owner = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_ST_OWNER,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->st_group) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->st_group);
				    ddbsec->st_group = sec->st_group;
				    sec->st_group = (char *)NULL;
				} else {
					if(ddbsec->st_group) {
				    		free(ddbsec->st_group);
				    		ddbsec->st_group = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_ST_GROUP,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->st_other) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->st_other);
				    ddbsec->st_other = sec->st_other;
				    sec->st_other = (char *)NULL;
				} else {
					if (ddbsec->st_other) {
				    		free(ddbsec->st_other);
				    		ddbsec->st_other = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_ST_OTHER,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->ual_enable) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->ual_enable);
				    ddbsec->ual_enable = sec->ual_enable;
				    sec->ual_enable = (char *)NULL;
				} else {
					if (ddbsec->ual_enable) {
				    		free(ddbsec->ual_enable);
				    		ddbsec->ual_enable=(char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
						       E_NOATTR,DDB_UAL_ENABLE,
						       sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->users) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->users);
				    ddbsec->users = sec->users;
				    sec->users = (char *)NULL;
				} else {
					if (ddbsec->users) {
				    		free(ddbsec->users);
				    		ddbsec->users = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR,DDB_USERS,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    if (sec->other) {
				if (cmd == DEV_MOD) {
				    free(ddbsec->other);
				    ddbsec->other = sec->other;
				    sec->other = (char *)NULL;
				} else {
					if(ddbsec->other) {
				    		free(ddbsec->other);
				    		ddbsec->other = (char *)NULL;
					} else {
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
							E_NOATTR, DDB_OTHER,
							sec->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
					}
				}
			    }

			    /* convert sec_entry to record (char *) */
			    if ((modrec=conv_secent(ddbsec)) == (char *)NULL) {
				err = TRUE;
				break;
			    }
			    /* write modified record to temp DDB_SEC file */
			    if (write_ddbrec(tmpfp, modrec) < 0) {
				err = TRUE;
				break;
			    }
			    free(modrec);	/* free memory - modrec */
			    freesecent(ddbsec); /* free memory - ddbsec*/
			    modified = TRUE;
			    continue;
			    /* Skip rest of the while loop.       *
			     * Read the next record from DDB_SEC. */
			} else if (cmp < 0 ) {
			    if (cmd == DEV_MOD) {
			    	/* sec_entry being added(new) for alias */
			    	/* convert sec_entry to record (char *) */
			    	if ((newrec=conv_secent(sec)) == (char *)NULL) {
					err = TRUE;
					break;
			    	}
			    	/* write new record to temp DDB_SEC file */
			    	if (write_ddbrec(tmpfp, newrec) < 0) {
					err = TRUE;
					break;
			    	}
			    	free(newrec);	/* free memory - newrec */
			    	modified = TRUE;
			   } else {
				free(rec);
				fclose(fp);
				fclose(tmpfp);
				rmtmpddb(tmpddb);
				ddb_errmsg(SEV_ERROR, EX_ERROR,E_ESSSEC,
					   sec->alias);
				return(FAILURE);
			   }
		        }
			*(--next) = ':';	/* replace ":" in record */
		    }
		    /* copy old record to temp file */
		    if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			break;
		    }
		    free(rec);	/* free memory - rec */
	} /* end while */

	/* if EOF reached on DDB_SEC file */
	if ((!modified)&&(!err)&&(cmd==DEV_MOD)) {
		    /* convert sec_entry to record (char *) */
		    if (newrec=conv_secent(sec)) {
			/* add new record to end of temp DDB_SEC file */
			if (write_ddbrec(tmpfp, newrec) < 0)
			    err = TRUE;
			free(newrec);	/* free memory - newrec */
		    } else  err = TRUE;
	}

	/* don't return failure if the error in the buffer is NOATTR */
	if ((err) || ((ddb_errget()>0) && (ddb_errget() != NOATTR))) {
			/* internal error, close all open files *
			* Delete temp DDB_SEC file             */
			fclose(fp);
			fclose(tmpfp);
			rmtmpddb(tmpddb);
	    		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			return(FAILURE);
	} 

	/* SUCCESS, close all files and return */
	fclose(fp);
	fclose(tmpfp);

	if (ddb_errget() == NOATTR) {
		return(NOATTR);
	}
	return(SUCCESS);
}

/*
 *  int rem_secent(salias)
 *	char	*salias;
 *
 *  This function removes the sec_entry defined for <salias> in DDB_SEC file,
 *  and puts the remaining entries in the temp DDB_SEC file -- <Tmp_ddbfile[]>.
 *
 *  Arguments:
 *	salias	secure device alias to be removed
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
rem_secent(salias)
char	*salias;
{
	FILE		*fp, *tmpfp;	/* file descriptors            */
	char		*tmpddb;	/* temp DDB_SEC filename       */
	sec_entry	*ddbsec;	/* sec_entry from DDB_SEC      */
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recalias;	/* alias in curr record        */
	int		removed;	/* removed or not             */
	int		cmp;		/* result of alias comparison  */
	int		err;		/* error flag                  */
	char		buf[80];	/* Where file's first line is 
					 * stored. Value is not used 	*/

	err = removed = FALSE;

	/* create temporary DDB_SEC file, open for write */
	if ( (tmpfp = opentmpddb(DDB_SEC, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_SEC file */
	    return(FAILURE);
	}

	/* set new magic number   */
	setmagicno(tmpfp);	

	/* open DDB_SEC file for read only */
	if ( (fp = fopen(DDB_SEC, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_SEC file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* skip magic no */
	if (fgets(buf,80,fp) == (char *)NULL) {
	        /* error reading DDB_SEC file*/
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	/* copy records from DDB_SEC to temp DDB_SEC file. *
	 * Remove the specified record in temp file.       */
	while (rec = read_ddbrec(fp)) {
		    if (!removed) {
			/* get alias field of record */
			recalias = getfield(rec,":",&next);

			/* compare alias with record alias */
			if ((cmp=strcmp(salias,recalias)) == 0) {
			    /* (new alias) = (record alias) *
			     * remove this record           */
			    free(rec);	/* free memory - rec */
			    removed = TRUE;
			    /* skip writing this record in temp DDB_SEC */
			    continue;
			}
			*(--next) = ':';	/* replace ":" in record */
		    }
		    /* copy old record to temp file */
		    if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			break;
		    }
		    free(rec);	/* free memory - rec */
	} /* end while */

	/* if EOF reached on DDB_SEC file */
	if ((err)||(ddb_errget())) {
		    /* internal error, close all open files *
		     * Delete temp DDB_SEC file             */
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
 *  static sec_entry *conv_secrec(rec)
 *	char        *rec;
 *
 *  This function converts the character string, <rec>,
 *  into the fields of a <sec_entry> structure. The memory for the
 *  <sec_entry> structure and individual attr values are malloc'ed 
 *  within this function.
 *
 *  Arguments:
 *	rec   	- DDB_SEC record (char string)
 *
 *  Returns: sec_entry *
 *	If it succeeds, it returns sec_entry, initialized with values 
 *		from <rec>
 *	If it fails, it returns a NULL pointer, error code set via
 *		ddb_errset().
 *
 */

static sec_entry *
conv_secrec(rec)
char	*rec;
{
	sec_entry	*sec;			/* ptr to sec_entry   */
	char		*recptr, *valbuf;	/* temp char pointers */
	char		*value, *next;		/* value & next value */
	int		fieldno;		/* rec field number   */
	int		reclen;

	recptr = rec;

	/* malloc space for sec_entry */
	sec=(sec_entry *)malloc(sizeof(sec_entry));
	if ( sec == (sec_entry *)NULL) {
	    /* error, could not malloc memory for sec_entry */
	    /* set ddb error code */
	    ddb_errset(errno);
	    return((sec_entry *)NULL);
	}

	INIT_SECENTRY(sec);

	/* extract attr values from rec and *
	 * initialize fields of sec_entry   */
	for (fieldno=1 ; fieldno<MAXSECATTRS ; fieldno++) {

		/* extract next attr value from rec */
		if (value=getfield(recptr, ":", &next)) {
		    if ((value)&&(*value)) {
			/* copy value into malloc'ed buffer */
			if (valbuf=(char *)malloc(strlen(value)+1)) {
			    strcpy(valbuf,value);
			} else {
			    /* error, ran out of memory */
			    ddb_errset(errno);
			    return((sec_entry *)NULL);
			}
		    } else {
			/* value -> NULL string */
			valbuf = (char *)NULL;
		    }
		    /* initialize corresponding field in sec_entry to value */
		    switch (fieldno) {
		    case (1):		/* attr = DDB_ALIAS           */
			sec->alias = valbuf;
			break;
		    case (2):		/* attr = DDB_RANGE           */
			sec->range = valbuf;
			break;
		    case (3):		/* attr = DDB_STATE           */
			sec->state = valbuf;
			break;
		    case (4):		/* attr = DDB_MODE            */
			sec->mode = valbuf;
			break;
		    case (5):		/* attr = DDB_STARTUP         */
			sec->startup = valbuf;
			break;
		    case (6):		/* attr = DDB_ST_LEVEL        */
			sec->st_level = valbuf;
			break;
		    case (7):		/* attr = DDB_ST_OWNER        */
			sec->st_owner = valbuf;
			break;
		    case (8):		/* attr = DDB_ST_GROUP        */
			sec->st_group = valbuf;
			break;
		    case (9):		/* attr = DDB_ST_OTHER        */
			sec->st_other = valbuf;
			break;
		    case (10):		/* attr = DDB_UAL_ENABLE      */
			sec->ual_enable = valbuf;
			break;
		    case (11):		/* attr = DDB_USERS           */
			sec->users = valbuf;
			break;
		    case (12):		/* attr = DDB_OTHER           */
			sec->other = valbuf;
			break;
		    default:
			return((sec_entry *)NULL);
		    }
		    /* bump recptr to next field */
		    recptr = next;
		}
	    }	/* end while */
	    if (value=getfield(next,"\n",&next)) {
		/* initialize last field in sec_entry */
		/* copy value into malloc'ed buffer */
		if (sec->other=(char *)malloc(strlen(value)+1)) {
		    strcpy(sec->other,value);
		} else {
		    /* error, ran out of memory */
		    ddb_errset(errno);
		    return((sec_entry *)NULL);
		}
	    } else {
		/* value -> NULL string */
		sec->other = (char *)NULL;
	    }
	/* return pointer to sec_entry */
	return (sec);
}

/*
 *  static char *conv_secent(sec)
 *	sec_entry       *sec;
 *
 *  This function converts the input sec_entry, <sec>, into a contiguous
 *  record, and return it as a character string. The memory for the
 *  record is malloc'ed within this function.
 *
 *  Arguments:
 *	sec   	- structure of type sec_entry
 *
 *  Returns: char *
 *	If it succeeds, it returns character string, with ":" separated
 *		values of fields extracted from <sec>.
 *	If it fails, it returns a NULL pointer, error code set via
 *		ddb_errset().
 *
 */

static char
*conv_secent(sec)
sec_entry	*sec;
{
	char		*recptr, *bufptr;	/* temp char pointers  */
	char		*value, *next;		/* value & next value  */
	int		fieldno;		/* rec field number    */
	int		bufsize;		/* size of buffer      */

	/* compute size of buffer required; include space for delimiter(:) */
	bufsize = strlen(sec->alias)+strlen(sec->range)+strlen(sec->state)
			+strlen(sec->mode)+strlen(sec->startup)
			+strlen(sec->st_level)+strlen(sec->st_owner)
			+strlen(sec->st_group)+strlen(sec->st_other)
			+strlen(sec->ual_enable)+strlen(sec->users)
			+strlen(sec->other)+MAXSECATTRS+1;
	
	/* malloc space for sec_entry */
	if (recptr=(char *)malloc(bufsize)) {
	    *recptr = '\0';
	    /* extract attr values from sec_entry and      *
	     * append(concatinate) them into record buffer */
	    recptr = strcat(recptr, sec->alias);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->range);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->state);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->mode);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->startup);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->st_level);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->st_owner);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->st_group);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->st_other);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->ual_enable);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->users);
	    recptr = strcat(recptr, ":");
	    recptr = strcat(recptr, sec->other);
	    recptr = strcat(recptr, "\n");
	} else {
	    /* set ddb error code */
	    ddb_errset(errno);
	    return((char *)NULL);
	}
	/* return pointer to record */
	return (recptr);
}

/*
 *  char *get_secattr(attr, field, sec)
 *	char        *attr;
 *	int         field;
 *	sec_entry   *sec;
 *
 *  This function returns the value of the specified <attr> from
 *  sec_entry, <sec>.
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	field	- field number in sec_entry.
 *	sec	- specified field of structure sec_entry,
 *		  is returned.
 *
 *  Returns: char *
 *	Value of attribute field is returned.
 *
 */
char *
get_secattr(attr, field, sec)
	char        *attr;
	int         field;
	sec_entry   *sec;
{
	char		*value;

	value = (char *)NULL;
	/* check if value and field number are defined  */
	if ((sec) && (field)) {
	    /* get corresponding field of sec_entry */
	    switch (field) {
	    case (0):			/* attr = ALIAS               */
		value = sec->range;
		break;
	    case (1):			/* attr = RANGE               */
		value = sec->range;
		break;
	    case (2):			/* attr = STATE               */
		value = sec->state;
		break;
	    case (3):			/* attr = MODE                */
		value = sec->mode;
		break;
	    case (4):			/* attr = DDB_STARTUP             */
		value = sec->startup;
		break;
	    case (5):			/* attr = DDB_ST_LEVEL       */
		value = sec->st_level;
		break;
	    case (6):			/* attr = STARTUP_OWNER       */
		value = sec->st_owner;
		break;
	    case (7):			/* attr = DDB_ST_GROUP       */
		value = sec->st_group;
		break;
	    case (8):			/* attr = DDB_ST_OTHER       */
		value = sec->st_other;
		break;
	    case (9):			/* attr = DDB_UAL_ENABLE     */
		value = sec->ual_enable;
		break;
	    case (10):			/* attr = DDB_USERS          */
		value = sec->users;
		break;
	    case (11):			/* attr = OTHER               */
		value = sec->other;
		break;
	    default:
		break;
	    }
	}
	return(value);
}

/*
 *  int getnextsec(fp, sec)
 *	FILE		*fp;
 *	char		sec[];
 *
 *  This function returns the next secure device alias from the DDB_SEC 
 *  file<fp>, in the specified buffer <sec[]>.
 *
 *  Arguments:
 *	fp	- File pointer to an open DDB_SEC file.
 *	sec	- pointer to output buffer of size DDB_MAXALIAS.
 *
 *  Returns: int
 *	SUCCESS Returns security device alias in <sec>.
 *	FAILURE NULL returned in <sec>, and error set using ddb_errset().
 *
 */

int
getnextsec(fp, sec)
	FILE		*fp;		/* DDB_SEC file pointer        */
	char		sec[];		/* secure device alias returned*/
{
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recalias;	/* alias in curr record        */
	int		err;		/* error flag                  */

	sec[0] = '\0';
	/* read next record from DDB_SEC */
	if (rec=read_ddbrec(fp)) {
	    recalias = getfield(rec,":",&next);
	    strncpy(sec, recalias, DDB_MAXALIAS);
	    free(rec);
	    return(SUCCESS);
	} else {
	    return(FAILURE);
	}
}

/*
 *  int make_secrem(attr, field, sec)
 *	char        *attr;
 *	int         field;
 *	sec_entry   *sec;
 *
 *  This function marks the specified <attr> in the sec_entry, <sec>
 *  for removal from the DDB_SEC file entry.
 *  The <value> used is "-"(Rem_secattr).
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	field	- field number in sec_entry.
 *	sec	- specified field of structure sec_entry,
 *		  initialized on return from function.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		if syntax error or invalid value
 *
 *  Notes:
 *    -	Given an attr=value string, the <field> number in sec_entry
 *	structure is obtained by inviking getattrtype().
 */
int
make_secrem(attr, field, sec)
	char        *attr;
	int         field;
	sec_entry   *sec;
{
	/* initialize corresponding field in sec_entry to Rem_secattr */
	switch (field) {
	    case (1):			/* attr = RANGE               */
		sec->range = Rem_secattr;
		break;
	    case (2):			/* attr = STATE               */
		sec->state = Rem_secattr;
		break;
	    case (3):			/* attr = MODE                */
		sec->mode = Rem_secattr;
		break;
	    case (4):			/* attr = DDB_STARTUP             */
		sec->startup = Rem_secattr;
		break;
	    case (5):			/* attr = DDB_ST_LEVEL       */
		sec->st_level = Rem_secattr;
		break;
	    case (6):			/* attr = STARTUP_OWNER       */
		sec->st_owner = Rem_secattr;
		break;
	    case (7):			/* attr = DDB_ST_GROUP       */
		sec->st_group = Rem_secattr;
		break;
	    case (8):			/* attr = DDB_ST_OTHER       */
		sec->st_other = Rem_secattr;
		break;
	    case (9):			/* attr = DDB_UAL_ENABLE          */
		sec->ual_enable = Rem_secattr;
		break;
	    case (10):			/* attr = DDB_USERS               */
		sec->users = Rem_secattr;
		break;
	    case (11):			/* attr = OTHER               */
		sec->other = Rem_secattr;
		break;
	    default:
		return(FAILURE);
	}
	return (SUCCESS);
}
