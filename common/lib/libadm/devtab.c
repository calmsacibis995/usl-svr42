/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/devtab.c	1.2.19.4"
#ident	"$Header: devtab.c 1.5 91/06/25 $"

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
 * E X T E R N A L   R E F E R E N C E S
 */
/*
 * Functions defined in devtab.c:
 *
 *	make_tabent()	makes tab_entry from input string
 *	get_tabent()	gets tab_entry of specified alias from DDB_TAB file
 *	put_tabent()	adds tab_entry to DDB_TAB file
 *	mod_tabent()	modifies entry in DDB_TAB file using tab_entry
 *	rem_tabent()	removes entry for specified alias in DDB_TAB file
 *	conv_tabent()	converts tab_entry into tab record (char *) 
 *	freetabent()	frees memory allocated for field of tab_entry 
 *	getaliasmap()	gets the alias type & # aliases mapped to device alias
 *	getalias()	gets the alias for the specified device from device.tab
 *			this function is provided for 4.0 compatibility
 *	_opendevtab()	Needed for compatibility with 4.0.
 *  	_devtabpath()   Needed for compatibility with 4.0
 */
extern FILE * oam_devtab;
/*
 *  L O C A L   D E F I N I T I O N S
 */

/*
 * Static Functions defined in devtab.c:
 *
 *	mkattrval()	makes an attrval struct from input attr & value pair
 *	conv_tabrec()	converts tab record (char *) into tab_entry *
 * 	getsecdev () 	returns the 4th entry after the first field in the
 *			string containing the record read from the device.tab
 */
static attr_val  *mkattrval();
static tab_entry *conv_tabrec();
static char      *getsecdev();

/*
 * Static Variables:
 * dtabrecnum      Record number of the current record (0 to n-1) -- Needed
 *		   by 4.0 functions _opendevtab
 */
static char	*Rem_tabattr = "-";	/* marker for dsf attr to be removed */
static int	dtabrecnum = 0;
/* 
 * Local definitions
 */
#define DTAB_PATH	"/etc/device.tab"

/*
 *  int make_tabent(attr, value, field, tab)
 *	char        *attr;
 *	char        *value;
 *	int         field;
 *	tab_entry   *tab;
 *
 *  This function validates the specified <value> for <attr>, and then
 *  initializes the specified <field> in the tab_entry, <tab>.
 *  If <value> is valid, it allocates memory for value string (malloc()).
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	value	- value of specified attribute.
 *	field	- field number in tab_entry.
 *	tab	- specified field of structure tab_entry,
 *		  initialized on return from function.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		if syntax error or invalid value
 *
 *  Notes:
 *    -	Given an attr=value string, the <field> number in tab_entry
 *	structure is obtained by invoking getattrtype().
 */
int
make_tabent(char *attr, char *value, int field, tab_entry *tab)
{
	char		*next;
	attr_val	*atval, *prevatval;

	/* NOTE:
	 * See make_dsfent for initialization of 
	 * tab->cdevice and tab->bdevice
	 */

	/* check if attr value is defined  */
	if (value) {
	    /* initialize corresponding field in tab_entry to value */
	    switch (field) {
     
	    case (0):		/* attr = alias ????? */

		/* ERROR, alias cannot be specified as an attribute */
		ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
		return(FAILURE);
		break;	

	    case (1):		/* attr = SECDEV */

		/* validate input secure device alias */
		if (!valid_alias(value)) 
		    /* error, invalid device alias */	
		    return(FAILURE);

		/* valid device alias, allocate memory */
		if (tab->secdev=(char *)malloc(strlen(value)+1)) {
			strcpy(tab->secdev, value);
		} else {
			/* ran out of memory */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
			return(FAILURE);
		}
		break;

	    case (2):			/* attr = PATHNAME            */

		/* pathname, allocate memory */
		if (!valid_path(value)) {
		    ddb_errmsg (SEV_ERROR, EX_INVAL, E_INVAL, DDB_PATHNAME);
		    return(FAILURE);
		}

		if (tab->pathname=(char *)malloc(strlen(value)+1)) {
		    strcpy(tab->pathname, value);
		} else {
		    /* ran out of memory */
		    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		    return(FAILURE);
		}
		break;
		
	    default:
		/* must be a new OA&M attribute */
		/* check if attr defined in attrlist */

		prevatval = (attr_val *)NULL;
		if (atval=tab->attrlist) do {
		    if (strcmp(atval->attr, attr) == 0) {
			/* attr multiply defined for device */
			ddb_errmsg(SEV_ERROR, EX_USAGE, E_MULTDEF, value, attr);
			return(FAILURE);
		    }
		    prevatval = atval;
		} while(atval=atval->next);

		if (atval=mkattrval(attr, value)) {
		    /* add entry to end of attr_val list */
		    if (prevatval) {
			prevatval->next = atval;
		    } else {
			tab->attrlist = atval;
		    }
		} else {
		    /* ran out of memory */
		    ddb_errmsg (SEV_ERROR, EX_INTPRB, E_NOMEM);
		    return(FAILURE);
		}
		break;
	    }	/* end switch */
	}
	return (SUCCESS);
}

/*
 *  void freetabent(tab)
 *	tab_entry       *tab;
 *
 *  This function free's the memory allocated for the input tab_entry, 
 *  <tab>, and its individual field values.
 *
 *  Arguments:
 *	tab   	- structure of type tab_entry
 *
 *  Returns: void
 *
 */
void
freetabent(tab_entry *tab)
{
	attr_val	*nextatval,	/* ptr to attr_val structs      */
			*curratval;

	/* free memory for the known fields of tab_entry */
	free(tab->alias);
	free(tab->cdevice);
	free(tab->bdevice);
	free(tab->pathname);
	free(tab->secdev);

	/* free memory for attr_val struct and fields */
	if (curratval=tab->attrlist) do {
		free(curratval->attr);
		free(curratval->val);
		nextatval = curratval->next;
		free(curratval);
	} while (curratval=nextatval);

	/* free memory for tab_entry struct */
	free(tab);
}

/*
 *  tab_entry get_tabent(alias)
 *	char        *alias;
 *
 *  This function returns in the structure <tab_entry>
 *  the attributes defined in the device.tab   for the specified <alias>.
 *
 *  Arguments:
 *	alias	- device alias name
 *
 *  Returns: tab_entry
 *	- Returns logical attrs in <tab_entry>. Memory for tab_entry
 *	  is allocated within this function.
 *	- Returns NULL ptr on ERROR, and sets error code(ddb_errset()).
 *
 */

tab_entry *
get_tabent(char *alias)
{
	FILE		*fp;		/* file pointer                */
	tab_entry	*tab;		/* device entry in DDB_TAB     */
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_TAB      */
	char		*recalias;	/* alias in curr record        */
	int		cmp;		/* result of alias comparison  */
	int		found;
	char		buf[80];	/* Where file's first line is read.
					 * The value is not used 	*/

	found = FALSE;
	tab = (tab_entry *)NULL;

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
            /* error, cannot open DDB_TAB file for read */
	    ddb_errset(errno);
	    return(NULL);
	}

	if (__tabversion__ ==  __4ES__) {
		/* skip magic number */
		if (fgets(buf, 80, fp) == (char *)NULL)  {
            		/* error reading DDB_TAB file */
	    		ddb_errset(errno);
			fclose(fp);
	    		return(NULL);
		}
	}

	/* read records from DDB_TAB */
	while ((!found) && (rec = read_ddbrec(fp))) {

		/* skip comments and empty lines */
		if (strchr("#\n", *rec) || isspace(*rec)) {
			free(rec);
			continue;
		}

		/* get alias field of record */
		recalias = getfield(rec,":",&next);

		/* compare alias with record alias */
		if ((cmp=strcmp(alias,recalias)) == 0) {
		    /* (alias) = (record alias) */

		    /* replace ':' in record */
		    *(--next) = ':';	

		    /* convert ddb record to tab_entry */
		    if (tab = conv_tabrec(rec)) {
			found = TRUE;
		    } else {
			/* internal error */
			ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);

			/* tab is set to NULL */
			fclose(fp);
			return(tab); 
		    } 
		}
		    
		free(rec);	/* free mem allocated for rec */

	} /* end while */

	/* check if entry for specified alias was found */
	if (!found) {
		ddb_errset(ENODEV);	/* device not found in DDB_TAB */
	} 
	
	fclose(fp);
	return(tab);
}

/*
 *  int put_tabent(tab)
 *	tab_entry   *tab;
 *
 *  This function creates a temporary file from the DDB_TAB file,
 *  and adds the new tab_entry <tab>, to that file.
 *
 *  It inserts the new entry <tab> into the temp DDB_TAB file, such that
 *  entries are ordered (ascending) by alias name. It also sets the
 *  new magic number on the temp DDB_TAB file -- <Tmp_ddbtab>.
 *
 *  Arguments:
 *	tab	- pointer to tab_entry
 *
 *  Global Static:
 *	char *Tmp_ddbtab	- name of the temp DDB_TAB file created.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		error
 *
 */

int
put_tabent(tab_entry *tab)
{
	FILE		*fp, *tmpfp;	/* file pointers               */
	char		*tmpddb;	/* ptr to temp DDB filename    */
	char		*next;		/* ptr to next field           */
	char		*newrec,	/* record written out to temp file */
			*conv_tabent();
	char		*rec;		/* curr record in DDB_TAB      */
	char		*recalias;	/* alias in curr record        */
	int		inserted;	/* inserted or not             */
	int		err;		/* error flag                  */
	char		buf[80];	/* Where TAB's first line is stored.
					 * This value is not used. 	*/

	/* convert tab_entry to record (char *) */
	if ((newrec = conv_tabent(tab)) == NULL) {
	    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    return(FAILURE);
	}

	err = inserted = FALSE;

	/* create temporary DDB_TAB file, open for write */
	if ((tmpfp = opentmpddb(DDB_TAB, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_TAB file */
	    return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {
		/* set new magic number   */
		setmagicno(tmpfp);	
	}

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_TAB file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {
		/* Skip magic number */
		if (fgets(buf, 80, fp) == (char *)NULL)  {
            		/* error reading DDB_TAB file */
	    		ddb_errset(errno);
			fclose(tmpfp);
			fclose(fp);
			rmtmpddb(tmpddb);
			return(FAILURE);
		}
	}	

	/* copy records from DDB_TAB to temporary DDB_TAB file.
	 * Insert new record in right place in temp DDB_TAB file 
	 */
	while (rec = read_ddbrec(fp)) {

		/* write to tmp files comments and empty lines */
		if (strchr("#\n", *rec) || isspace(*rec)) {
			if (write_ddbrec(tmpfp,rec) < 0) {
				err = TRUE;
				free(rec);
				break;
			}
			free(rec);
			continue;
		}
		if (!inserted) {
			/* get alias field of record */
			recalias = getfield(rec,":",&next);

			/* compare alias with record alias */
			if (strcmp(tab->alias,recalias) < 0) {
				
		    		/* (new alias) < (record alias) *
		     	 	* insert new record right here */
		    		if (write_ddbrec(tmpfp, newrec) < 0) {
					err = TRUE;
					free(rec);
					free(newrec);
					break;
		    		}

				/* free mem alloc'ed for newrec */
		    		free(newrec);   
		    		inserted = TRUE;
			}

			/* replace ":" in record */
			*(--next) = ':';	
	 	}
		/* copy old record to temp file */
		if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			free(rec);
			break;
		}

		free(rec);	/* free mem alloc'ed for old rec */
	} /* end while */

	/* if EOF reached on DDB_TAB file */
	if ((!inserted)&&(!err)) {

		/* add new record to end of temp DDB_TAB file */
		if (write_ddbrec(tmpfp, newrec) < 0)
			err = TRUE;

		/* free mem alloc'ed for newrec */
	    	free(newrec);	
	}

	if ((err)||(ddb_errget())) {
		    /* internal error, delete temp DDB_TAB file */
		    fclose(tmpfp);
		    fclose(fp);
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
 *  int mod_tabent(tab, cmd)
 *	tab_entry   *tab;
 *	int         cmd;
 *
 *  This function modifies the tab_entry <tab>, defined in DDB_TAB file,
 *  and puts the updated entry in the temp DDB_TAB file -- <Tmp_ddbtab>.
 *  If <cmd> = DEV_MOD, then it modifies the non-NULL fields(attr-values)
 *			in the tab_entry in DDB_TAB file.
 *  If <cmd> = DEV_REM, then it removes the non-NULL fields (attr-values)
 *			from the tab_entry in DDB_TAB file.
 *
 *  Arguments:
 *	tab	- pointer to tab_entry
 *	cmd	- DEV_MOD or DEV_REM
 *
 *  Global Static:
 *	char *Tmp_ddbtab	- name of the temp DDB_TAB file created.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		error
 *	NOATTR	if trying to remove an attribute that is not defined
 *
 */

int
mod_tabent(tab_entry *tab, int cmd)
{
	FILE		*fp, *tmpfp;	/* file descriptors            */
	char		*tmpddb;	/* temp DDB_TAB filename       */
	tab_entry	*ddbtab;	/* tab_entry from DDB_TAB      */
	char		*next;		/* ptr to next field           */
	char		*modrec,	/* record written out to temp file */
			*conv_tabent();	/* converts tab_entry to rec   */
	char		*rec;		/* curr record in DDB_TAB      */
	char		*recalias;	/* alias in curr record        */
	attr_val	*old, *new,	/* old & new attr_val list ptrs */
			*prevold, *nextnew;
	int		modified;	/* modified or not             */
	int		cmp;		/* result of alias comparison  */
	int		err;		/* error flag                  */
	int		found;
	char		buf[80];	/* Where DDB_TAB's first line is
					 * stored. Value is ignored. */
	err = modified = found = FALSE;

	/* create temporary DDB_TAB file, open for write */
	if ((tmpfp = opentmpddb(DDB_TAB, &tmpddb)) == (FILE *)NULL) {
	    /* error, cannot create temp DDB_TAB file */
	    return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {
		/* set new magic number   */
		setmagicno(tmpfp);	
	}

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_TAB file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {
		/* Skip over magic number */
		if (fgets(buf, 80, fp) == (char *)NULL)  {
         		/* error reading DDB_TAB file */
	    		ddb_errset(errno);
			fclose(tmpfp);
			fclose(fp);
			rmtmpddb(tmpddb);
	    		return(FAILURE);
		}
	}

	/* copy records from DDB_TAB to temp DDB_TAB file
	 * Insert modified record in right place.
	 */
	while (rec = read_ddbrec(fp)) {

		/* write comments and empty lines to temporary file*/
		if (strchr("#\n", *rec) || isspace(*rec)) {
			if (write_ddbrec(tmpfp,rec) < 0) {
				err = TRUE;
				free(rec);
				break;
			}
			free(rec);
			continue;
		}

		if (!modified) {
			/* get alias field of record */
			recalias = getfield(rec,":",&next);

			/* compare alias with record alias */
			if ((cmp=strcmp(tab->alias,recalias)) == 0) {
			    /* (new alias) = (record alias) *
			     * modify this record           */
			    *(--next) = ':';	/* replace ':' in record */

			    /* convert ddb record to tab_entry */
			    ddbtab = conv_tabrec(rec);
			    if (ddbtab ==  (tab_entry *)NULL) {
				ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
				fclose(tmpfp);
				fclose(fp);
				rmtmpddb(tmpddb);
				return(FAILURE);
			    }

			    /* For cmd= DEVMOD: 
			     * If input <tab> field not NULL, then copy
			     * new field value from <tab> to <ddbtab> 
			     * For cmd= DEVREM:
			     * If input <tab> field not NULL, then remove
			     * existing field value from <ddbtab>
			     */

			   if ((_mac_installed())&&(tab->secdev)) {
				if (cmd == DEV_MOD) {
				    free(ddbtab->secdev);
				    ddbtab->secdev = tab->secdev;
				    tab->secdev = (char *)NULL;
				} else {
				    free(ddbtab->secdev);
				    ddbtab->secdev = ddbtab->alias;
				}
			   }

			   if (tab->cdevice) {
				if (cmd == DEV_MOD) {
				    free(ddbtab->cdevice);
				    ddbtab->cdevice = tab->cdevice;
				    tab->cdevice = (char *)NULL;
				} else {
				    free(ddbtab->cdevice);
				    ddbtab->cdevice = (char *)NULL;
				}
			   }

			   if (tab->bdevice) {
				if (cmd == DEV_MOD) {
				    free(ddbtab->bdevice);
				    ddbtab->bdevice = tab->bdevice;
				    tab->bdevice = (char *)NULL;
				} else {
				    free(ddbtab->bdevice);
				    ddbtab->bdevice = (char *)NULL;
				}
			   }

			   if (tab->pathname) {
				if (cmd == DEV_MOD) {
				    free(ddbtab->pathname);
				    ddbtab->pathname = tab->pathname;
				    tab->pathname = (char *)NULL;
				} else {
				    free(ddbtab->pathname);
				    ddbtab->pathname = (char *)NULL;
				}
			    }

			    prevold = (attr_val *)NULL;
			    
			    /* if there are OA&M attributes defined
			     * for the device, check them
			     */
			    if (old=ddbtab->attrlist) {
				/* check if attr defined in attrlist */
				if (new=tab->attrlist) do {
				    while ((!found) && 
				     (old != (attr_val *) NULL)) {
					if (strcmp(old->attr, new->attr) == 0) {
					    found = TRUE;
					    if(cmd==DEV_MOD) {
						free(old->val);
						old->val = new->val;
						new->val = (char *)NULL;
					    } else {
						/*Delete entry from attrlist*/
						free(old->val);
						free(old->attr);
						old->val = (char *)NULL;
						old->attr = (char *)NULL;
						if (prevold != (attr_val *)NULL) {
							prevold->next=old->next;
							old=prevold->next;
						}
						else {
							/* item is the first
							 * one in the list */
							if (old->next != (attr_val *)NULL) {

								ddbtab->attrlist=old->next;
								old = ddbtab->attrlist;
							}else {
								/* item was the
								 * only one in
							 	 * list. Set
							 	 * list to point
							 	 * to NULL */
								ddbtab->attrlist = (attr_val *) NULL;
							}

						}
						continue;
					    }
					}
					prevold = old;
					old=old->next;

				    }	/* end for loop */
				    /*
				     * If not found, add new attr to <old> list.
				     * Else bump to next attr to look for.
				     */
				    if (!found) {
					if (cmd == DEV_MOD) {
					/* add new attrval to end of list */
						prevold->next = new;
						nextnew = new->next;
						new->next = (attr_val *)NULL;
						new = nextnew;
					} else {
					/* For 4.0 compatibility:
					 * display diagnostic but
					 * continue with next attribute
					 * return failure to invoking function
					 */
						ddb_errmsg(SEV_ERROR, EX_NOATTR,
						 E_NOATTR,new->attr,tab->alias);
						err_report(Cmdname, ACT_CONT);
						ddb_errset(NOATTR);
						new = new->next;
						found =  FALSE;
					 }
				    } else {

					/* found, bump to next new */
					new = new->next;
					found =  FALSE;
				   }

				  /* set old to the beginning of attrlist */
				  old = ddbtab->attrlist;
				  prevold = (attr_val *)NULL;

				/* while more attrs to modify */
				} while (new != (attr_val *)NULL); 

			    } else {

				/* device had no OAM attributes
				 * add entire list, if cmd is modify */
				if (cmd == DEV_MOD)
					ddbtab->attrlist = tab->attrlist;
				else {
					/* trying to remove OA&M attributes
					 * that are not defined. Issue a 
					 * warning for each one, if any
					 */
					new=tab->attrlist;
					if (new) {
						while (new != (attr_val *)NULL) {
							ddb_errmsg(SEV_ERROR, EX_NOATTR,
					 	 	E_NOATTR,new->attr,tab->alias);
							err_report(Cmdname, ACT_CONT);
							new=new->next;
						}
						ddb_errset(NOATTR);
					}
				}
			    }

			    /* done processing all fields */

			    /* convert tab_entry to record (char *) */
			    if ((modrec=conv_tabent(ddbtab)) == (char *)NULL) {
				err = TRUE;
				break;
			    }

			    /* write modified record to temp DDB_TAB file */
			    if (write_ddbrec(tmpfp, modrec) < 0) {
				err = TRUE;
				break;
			    }

			    free(modrec);
			    freetabent(ddbtab);

			    modified = TRUE;

			    /* Skip rest of the while loop.       *
			     * Read the next record from DDB_TAB. */
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

	if ((err) || (ddb_errget() >0 && ddb_errget() != NOATTR)) {
	    	/* internal error, close all open
		 * files Delete temp DDB_TAB file  */
	    	fclose(fp);
	    	fclose(tmpfp);
	    	rmtmpddb(tmpddb);
	  	ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
	   	return(FAILURE);
	}

	/* SUCCESS, close all files and return */
	fclose(fp);
	fclose(tmpfp);
	if (ddb_errget() == NOATTR)
		return(NOATTR);
	return(SUCCESS);
}

/*
 *  int rem_tabent(tab)
 *	char   *alias;
 *
 *  This function removes the tab_entry for specified <alias>
 *  defined in DDB_TAB file and copies the remaining entries into the 
 *  temp DDB_TAB file -- <Tmp_ddbtab>.
 *
 *  Arguments:
 *	alias	- alias to be removed.
 *
 *  Global Static:
 *	char *Tmp_ddbtab	- name of the temp DDB_TAB file created.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		error
 *
 */

int
rem_tabent(char *alias)
{
	FILE		*fp, *tmpfp;	/* file descriptors            */
	char		*tmpddb;	/* temp DDB_TAB filename       */
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_TAB      */
	char		*recalias;	/* alias in curr record        */
	int		removed;	/* removed or not             */
	int		cmp;		/* result of alias comparison  */
	int		err;		/* error flag                  */
	char		buf[80];	/* Were DDB_TAB's first line is stored.
					 * This value is ignored.	*/

	err = removed = FALSE;

	/* create temporary DDB_TAB file, open for write */
	if ( (tmpfp = opentmpddb(DDB_TAB, &tmpddb)) == (FILE*)NULL) {
	    /* error, cannot create temp DDB_TAB file */
	    return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {
		/* set new magic number   */
		setmagicno(tmpfp);	
	}

	/* open DDB_TAB file for read only */
	if ( (fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
	        /* error, cannot open DDB_TAB file for read */
		fclose(tmpfp);
		rmtmpddb(tmpddb);
		return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {

		/* Skip magic number */
		if (fgets(buf, 80, fp) == (char *)NULL)  {
        		/* error reading DDB_TAB file */
	    		ddb_errset(errno);
			fclose(fp);
			fclose(tmpfp);
			rmtmpddb(tmpddb);
			return(FAILURE);
		}
	}
	/* Copy records from DDB_TAB to temp DDB_TAB file.
	 * Remove the specified record in temp file       */
	while (rec = read_ddbrec(fp)) {

		/* write to temp file comments and empty lines */
		if (strchr("#\n", *rec) || isspace(*rec)) {
			if (write_ddbrec(tmpfp, rec)<0) {
				err = TRUE;
				free(rec);
				break;
			}
			free(rec);
			continue;
		}

		if (!removed) {
			/* get alias field of record */
			recalias = getfield(rec,":",&next);

			/* compare alias with record alias */
			if ((cmp=strcmp(alias,recalias)) == 0) {

			    	/* (new alias) = (record alias) *
			     	 * remove this record           */
			    	free(rec);	/* free memory - rec */
			    	removed = TRUE;

			    	/* skip writing this record in Tmp_ddbtab */
			    	continue;
			}

			/* replace ":" in record */
			*(--next) = ':';	
		}

		/* copy old record to temp file */
		if (write_ddbrec(tmpfp, rec) < 0) {
			err = TRUE;
			free(rec);
			break;
		}

		/* free memory - rec */
		free(rec);	
		
	} /* end while */

	/* if EOF reached on DDB_TAB file */
	if ((err)||(ddb_errget())) {
		    /* internal error, close all open files *
		     * Delete temp DDB_TAB file             */
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
 *  static tab_entry *conv_tabrec(rec)
 *	char  *rec;
 *
 *  This function converts the character string, <rec>,
 *  into the fields of a <tab_entry> structure, taking into
 *  consideration if the device.tab is a 4.0 version or 4ES.
 *  The memory for the <tab_entry> structure and
 *  individual attr values are malloc'ed within this function.
 *
 *  Arguments:
 *	rec   	- DDB_TAB record (char string)
 *
 *  Returns: tab_entry *
 *	If it succeeds, it returns tab_entry, initialized with values 
 *		from <rec>
 *	If it fails, it returns a NULL pointer, error code set via
 *		ddb_errset().
 *
 */

static tab_entry *
conv_tabrec(rec)
char * rec; 
{
	tab_entry	*tab;			/* ptr to tab_entry   */
	char		*recptr, *valbuf;	/* temp char pointers */
	char		*attr;			/* attr name          */
	char		*value, *next;		/* value & next value */
	int		fieldno;		/* rec field number   */
	attr_val	*prevatval, *nextatval;
	int		reclen;

	recptr = rec;

	/* malloc space for tab_entry */
	tab=(tab_entry *)malloc(sizeof(tab_entry));

	if (tab == (tab_entry *)NULL) {
	    /* error, could not malloc memory for tab_entry */
	    /* set ddb error code */
	    ddb_errset(errno);
	    return((tab_entry *)NULL);
	}

	/* Initialize the entries for tab_entry */
	INIT_TABENTRY(tab);

	fieldno = 0;


	/* The first four fields in the device.tab 
	 * appy to both 4.0 and 4ES format.
	 */
	while (fieldno <= 3 ) {
	
		/* extract next attr value from rec */
		if (value=getfield(recptr, ":", &next)) {
		
			if ((value)&&(*value)) {
	
				/* copy value into malloc'ed buffer */
				valbuf=(char *)malloc(strlen(value) + 1);
				if (valbuf) {
		    			strcpy(valbuf,value);
				} else {
		    			/* error, ran out of memory */
		    			ddb_errset(errno);
		    			return((tab_entry *)NULL);
				}

		    	} else {
				/* value -> NULL string */
				valbuf = (char *)NULL;
		    	}

		    	switch (fieldno) {
		    	case (0):		/* attr = ALIAS */
				tab->alias = valbuf;
				break;
		    	case (1):		/* attr = CDEVICE */
				tab->cdevice = valbuf;
				break;
		    	case (2):		/* attr = BDEVICE */
				tab->bdevice = valbuf;
				break;
		    	case (3):		/* attr = PATHNAME */
				tab->pathname = valbuf;
				break;
		    	}

		    	/* bump recptr to next field */
		    	recptr = next;
		    	fieldno++;

		}  /* if value = getfield */
	}	/* end while */

	/* If 4ES, leave room for secdev field */
	if (__tabversion__ == __4ES__) {
		/* get the secdev field */
		if (value=getfield(next, ":", &next)) {
			if (*value) {
				valbuf = (char *)malloc(strlen(value) + 1);
				if (valbuf) {
		    			strcpy(valbuf,value);
				} else {
		    			/* error, ran out of memory */
		    			ddb_errset(errno);
		    			return((tab_entry *)NULL);
				}
			} else
				valbuf = (char *)NULL;
			tab->secdev = valbuf;
		}
	}

	/* if there are more attrs defined in DDB_TAB record */
	if ((*next) && (*next != '\n')) {

		/* extract next attr & value pair   */
		if ((attr=getfield(next, "=", &next)) &&
			(value=getquoted(next, &next))) {

		    /* initialize attr_val list elements */
		    if (nextatval=mkattrval(attr, value)) {
			tab->attrlist = prevatval = nextatval;

			/* process rest of the attrs until '\n' */
			while ((*next)&&(*next != '\n')) {

			    /* extract next attr & value pair   */
			    if(((attr=getfield(next, "=", &next))!=NULL) &&
				((value=getquoted(next, &next))!=NULL)) {

				/* initialize attr_val list elements */
				if (nextatval=mkattrval(attr, value)) {
				    prevatval->next = nextatval;
				    prevatval = nextatval;
				} else {
				    return((tab_entry *)NULL);
				}

			    } else {
				return((tab_entry *)NULL);
			    }

			}	/* end while */

		    } else {
			return((tab_entry *)NULL);
		    }

		} else {
		    return((tab_entry *)NULL);
		}

	    } else tab->attrlist = (attr_val *)NULL;


	/* return pointer to tab_entry */
	return (tab);
}

/*
 *  static char *conv_tabent(tab)
 *	tab_entry       *tab;
 *
 *  This function converts the input tab_entry, <tab>, into a contiguous
 *  record, and return it as a character string. The memory for the
 *  record is malloc'ed within this function.
 *
 *  Arguments:
 *	tab   	- structure of type tab_entry
 *
 *  Returns: char *
 *	If it succeeds, it returns character string, with ":" separated
 *		values of fields extracted from <tab>.
 *	If it fails, it returns a NULL pointer, error code set via
 *		ddb_errset().
 *
 */

static char *
conv_tabent(tab_entry *tab)
{
	char		*recptr, *bufptr;	/* temp char pointers  */
	char		*value, *next;		/* value & next value  */
	attr_val	*nextatval;		/* ptr to attr_val struct */
	int		fieldno;		/* rec field number    */
	int		bufsize;		/* size of buffer      */

	/* compute size of buffer required: the string will have the following 
	 * format-> alias:cdevice:bdevice:pathname:secdev: oam attributes
	 * Add five for the five delimiters(:), one for the \n and one for
	 * the \0
	 */
	bufsize=strlen(tab->alias)+strlen(tab->cdevice)+strlen(tab->bdevice)+
		strlen(tab->pathname)+strlen(tab->secdev)+ 5 + 1 + 1;

	if (nextatval=tab->attrlist) do {
		/* compute memory reqd for each attr-val string */
		bufsize += strlen(nextatval->attr) + strlen(nextatval->val) + 4;
		nextatval = nextatval->next;
	} while (nextatval);
	

	/* malloc space for record */
	if ( (recptr=(char *)malloc(bufsize)) == (char *)NULL) {
	    /* set ddb error code */
	    ddb_errset(errno);
	    return((char *)NULL);
	}

	*recptr = '\0';

	/* Extract attr values from tab_entry and      *
	 * append them into record buffer 
	 */

	recptr = strcpy(recptr, tab->alias);
	recptr = strcat(recptr, ":");
	recptr = strcat(recptr, tab->cdevice);
	recptr = strcat(recptr, ":");
	recptr = strcat(recptr, tab->bdevice);
	recptr = strcat(recptr, ":");
	recptr = strcat(recptr, tab->pathname);
	recptr = strcat(recptr, ":");

	if (__tabversion__ == __4ES__) {
		recptr = strcat(recptr, tab->secdev);
		recptr = strcat(recptr, ":");
	}

	if (nextatval=tab->attrlist) do {
		/* extract attr-val and concatenate it to record */
		recptr = strcat(recptr, " ");
		recptr = strcat(recptr, nextatval->attr);
		recptr = strcat(recptr, "=");
		recptr = strcat(recptr, "\"");
		recptr = strcat(recptr, nextatval->val);
		recptr = strcat(recptr, "\"");
		nextatval = nextatval->next;
	} while (nextatval);

	/* append '\n' character at end of line */
	recptr = strcat(recptr, "\n");

	/* return pointer to record */
	return (recptr);
}

/*
 *  int getaliasmap(alias, atype, mapcnt, secdev)
 *	char	*alias;
 *	int	*atype;
 *	int	*mapcnt;
 *	char	*secdev;
 *
 *  This function searches for the specified <alias> in the 
 *  "alias"(1st field) and "secdev"(5th field), of every one of the entries
 *  in DDB_TAB file. If it finds it in the "alias" field, it returns a
 *  value of 1, else it returns a value of 0.
 *
 *  In addition, it returns the alias type in <atype>, the number of
 *  aliases defined in DDB_TAB that map to the specified alias in <mapcnt>,
 *  and the <secdev> attribute value defined for <alias> in DDB_TAB.
 *
 *  Arguments:
 *	alias  	- alias name to be searched for in DDB_TAB file.
 *	atype	- alias type -- DEV_ALIAS - if found in "alias" field.
 *	                        DEV_SECDEV- if found in "secdev" field.
 *	mapcnt	- no. of entries where <alias> found in "secdev" field.
 *	secdev	- secdev attr value, if found in "alias" field of DDB_TAB.
 *
 *  Returns: int
 *	TRUE	- if <alias> found in "alias" field.
 *	FALSE	- if <alias> not found in "alias" field of any entry.
 *	-1	- if error occurs.
 *
 */

int
getaliasmap(char *alias, int *atype, int *mapcnt, char *secdev)
{
	FILE	*fp;
	char	*rec;
	char	*al, *s_al, *next;
	int	cmp, found, err, i;
	char	buf[80];	/* Where file's first line is stored.
				 * This value is not used. */
	err = found = FALSE;
	*mapcnt = *atype = 0;

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
            /* error, cannot open DDB_TAB file for read */
	    return(FAILURE);
	}

	if (__tabversion__ == __4ES__) {
		if (fgets( buf, 80, fp) == (char *)NULL) { 
			ddb_errset(errno);
			fclose(fp);
			return(FAILURE);
		}
	}

	/* read records from DDB_TAB */
	while (rec = read_ddbrec(fp)) {

		/* skip comments and empty lines */
		if (strchr("#\n", *rec) || isspace(*rec)) {
			free(rec);
			continue;
		}

		/* get alias field of record */
		al = getfield(rec,":",&next);

		/* get secdev field of record */
		s_al = getsecdev(next);

		/* compare alias with record alias */
		if ((cmp=strcmp(alias,al)) == 0) {

		    /* (alias) = (record alias) */
		    *atype = DEV_ALIAS;


		    if (__tabversion__== __4ES__) {

		    	/* copy <secdev> value into output buffer */
		    	strncpy(secdev, s_al, DDB_MAXALIAS);
		    	if (strcmp(alias, s_al) == 0) {
				/* (alias) = (record secdev) */
				*atype = DEV_SECDEV;
		    	}

		    }
		    found = TRUE;
		} else  {
			if (__tabversion__ == __4ES__) {

				if (strcmp(alias, s_al) == 0) {

		    			/* (alias) = (record secdev)
		     		 	 * Another logical alias maps 
				 	 * to specified <alias>. thus <alias>
		     		 	 * must be of type DEV_SECDEV  
				 	 */
		    			*atype = DEV_SECDEV;
		    			(*mapcnt)++;
				}
			}
		}
		free(rec);	/* free mem allocated for rec */

	} /* end while */

	/* check if entry for specified alias was found */
	if (ddb_errget()) {
		fclose(fp);
		return(FAILURE);
	}

	/* SUCCESS, return found or not found */
	fclose(fp);
	return(found);
}

/*
 *  attr_val *mkattrval(attr, value)
 *	char   *attr;
 *      char   *value;
 *
 *	This function returns an attr_val structure, with its fields
 *	initialized to the input values <attr> and <val>.
 *
 *  Arguments:
 *	char *attr	attribute name
 *	char *value	value string
 *
 *  Returns:  struct attr_val *
 *	The address of a malloc()ed structure containing the attribute 
 *	name and the value string.
 */

static attr_val *
mkattrval(char *attr, char *value)
{
	attr_val	*atval;

	atval = (attr_val *)NULL;

	if (attr) {

	    if (atval=(attr_val *)malloc(sizeof(attr_val))) {

		/* alloc memory for fields of attr_val struct */
		if((atval->attr=(char *)malloc(strlen(attr)+1)) &&
			(atval->val=(char *)malloc(strlen(value)+1))) {

		    /* initialize fields of attr_val struct */
		    strcpy(atval->attr, attr);
		    strcpy(atval->val, value);
		    atval->next = (attr_val *)NULL;
		} else {
		    /* ran out of memory */
		    ddb_errset (ENOMEM);
		    return((attr_val *)NULL);
		}
	    } else {
		/* ran out of memory */
		ddb_errset (ENOMEM);
		return((attr_val *)NULL);
	    }
	}
	return(atval);
}

/*
 *  char *get_tabattr(attr, field, tab)
 *	char        *attr;
 *	int         field;
 *	tab_entry   *tab;
 *
 *  This function returns the value of the specified <attr> from
 *  tab_entry, <tab>.
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	field	- field number in tab_entry.
 *	tab	- specified field of structure tab_entry,
 *		  is returned.
 *
 *  Returns: char *
 *	Value of attribute field is returned.
 *
 */
char *
get_tabattr(attr, field, tab)
	char        *attr;
	int         field;
	tab_entry   *tab;
{
	char		*value;
	attr_val	*atval;

	value = (char *)NULL;

	/* NOTE:
	 * tab->cdevice and tab->bdevice are obtained when
	 * calling get_dsfattr
	 */

	/* check if value and field number are defined  */
	if (tab) {
	    /* get corresponding field of tab_entry */
	    switch (field) {
	    case (0):			/* attr = ALIAS               */
		value = tab->alias;
		break;
	    case (1):			/* attr = SECDEV              */
		value = tab->secdev;
		break;
	    case (2):			/* attr = PATHNAME            */
		value = tab->pathname;
		break;
	    default:
		if (atval=tab->attrlist) do {
		    if (strcmp(atval->attr, attr) == 0) {
			value = atval->val;
			break;
		    }
		} while(atval=atval->next);
		break;
	    }
	}
	return(value);
}

/*
 *  int make_tabrem(attr, field, tab)
 *	char        *attr;
 *	int         field;
 *	tab_entry   *tab;
 *
 *  This function marks the specified attributes <attr> for removal
 *  from the tab_entry in the DDB_TAB file.
 *  If <value> used is "-"(Rem_tabattr- a static variable).
 *
 *  Arguments:
 *	attr	- attribute name char string.
 *	field	- field number in tab_entry.
 *	tab	- specified field of structure tab_entry,
 *		  initialized on return from function.
 *
 *  Returns: int
 *	SUCCESS		if all goes well
 *	FAILURE		if syntax error or invalid value
 *
 */
int
make_tabrem(attr, field, tab)
char		*attr;
int		field; 
tab_entry	*tab;
{
	attr_val	*atval, *prevatval;

	/* NOTE:
	 * if tab->cdevice or tab->bdevice are being removed
	 * the initalization to "-" is done by make_dsfrem()
	 */
	 
	/* initialize corresponding field in tab_entry to value */
	switch (field) {
	    case (0):
		/* ERROR, alias cannot be specified as an attribute */
		ddb_errmsg(SEV_ERROR, EX_USAGE, E_USAGE);
		return(FAILURE);
		break;	
	    case (1):			/* attr = SECDEV              */
		tab->secdev = Rem_tabattr;
		break;
	    case (2):			/* attr = PATHNAME            */
		tab->pathname = Rem_tabattr;
		break;
	    default:
		/* must be other OA&M attribute */
		/* check if attr defined in attrlist */
		prevatval = (attr_val *)NULL;
		if (atval=tab->attrlist) do {
		    if (strcmp(atval->attr, attr) == 0) {
			/* attr already marked for removal */
			return(SUCCESS);
		    }
		    prevatval = atval;
		} while(atval=atval->next);

		if (atval=mkattrval(attr, Rem_tabattr)) {
		    /* add entry to end of attr_val list */
		    if (prevatval) {
			prevatval->next = atval;
		    } else {
			tab->attrlist = atval;
		    }
		} else {
		    /* ran out of memory */
		    ddb_errmsg (SEV_ERROR, EX_INTPRB, E_NOMEM);
		    return(FAILURE);
		}
		break;
	    }	/* end switch */
	return (SUCCESS);
}

/*
 *  int getnextdev(fp, dev)
 *	FILE		*fp;
 *	char		dev[];
 *
 *  This function returns the next device alias from the DDB_TAB 
 *  file<fp>, in the specified buffer <dev[]>.
 *
 *  Arguments:
 *	fp	- File pointer to an open DDB_TAB file.
 *	dev	- pointer to output buffer of size DDB_MAXALIAS.
 *
 *  Returns: int
 *	SUCCESS Returns device alias in <dev>.
 *	FAILURE NULL returned in <sec>, and error set using ddb_errset().
 *
 */

int
getnextdev(fp, dev)
	FILE		*fp;		/* DDB_TAB file pointer        */
	char		dev[];		/* device alias returned       */
{
	char		*next;		/* ptr to next field           */
	char		*rec;		/* curr record in DDB_SEC      */
	char		*recalias;	/* alias in curr record        */
	int		err;		/* error flag                  */

	dev[0] = '\0';
	/* read next record from DDB_TAB */
	while(rec=read_ddbrec(fp)) {
		/* skip comments and empty lines */
		if (strchr("#\n", *rec) || isspace(*rec)) {
			free(rec);
			continue;
		}
	    	recalias = getfield(rec,":",&next);
	    	strncpy(dev, recalias, DDB_MAXALIAS);
	    	free(rec);
	    	return(SUCCESS);
	}
	return(FAILURE);
}
/*
 *  tab_entry * getalias(device)
 *	char		*device;
 *	int		*dtype;
 *
 *  This function returns tab_entry * containing the device.tab
 *  entry for which there was a match between <device> and
 *  cdevice, bdevice, or pathname
 *
 *  Arguments:
 *	device  - pointer to device for which the information is wanted
 *
 *  Returns: int
 *	SUCCESS if tab_entry was successfully initialized
 *	FAILURE is returned if device not found on TAB, or a problem is
 *      	encountered
 *
 */
tab_entry *
getalias(device,dtype)
char *device;
int  *dtype;
{
	char *rec;
	FILE * fp;
	int found = FALSE;
	tab_entry *tab = (tab_entry *)NULL;

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
            /* error, cannot open DDB_TAB file for read */
	    return(tab);
	}
	while (rec = read_ddbrec(fp)) {
		/* skip comments and empty lines */
		if (strchr("#\n", *rec) || isspace(*rec)) {
			free(rec);
			continue;
		}
		tab = conv_tabrec(rec);
		if ((strcmp(tab->cdevice,device) == 0)) {
			*dtype = CDEV;
			free(rec);
			found = TRUE;
			break;
		}
		if ((strcmp(tab->bdevice, device) == 0)) {
			*dtype = BDEV;
			free(rec);
			found = TRUE;
			break;
		}
		if ((strcmp(tab->pathname, device) == 0)) {
			*dtype = 0;
			free(rec);
			found = TRUE;
			break;
		}
		free(rec);
		free(tab);
	}
	fclose(fp);
	if (found)
		return(tab);
	return((tab_entry *)NULL);
}
/*
 *  int _opendevtab(mode)
 *	char   *mode
 *
 *	The _opendevtab() function opens a device table for a command.
 *
 *  Arguments:
 *	mode	The open mode to use to open the file.  (i.e. "r" for
 *		reading, "w" for writing.  See FOPEN(BA_OS) in SVID.)
 *
 *  Returns:  int
 *	TRUE if it successfully opens the device table file, FALSE otherwise
 */

int
_opendevtab(mode)
	char   *mode;
{
	/*
	 *  Automatic data
	 */

	char   *devtabname;		/* Ptr to the device table name */
	int	rtnval;			/* Value to return */


	rtnval = TRUE;
	if (devtabname = _devtabpath()) {
	    if (oam_devtab) (void) fclose(oam_devtab);
	    if (oam_devtab = fopen(devtabname, mode))
		dtabrecnum = 0;  /* :-) */
	    else rtnval = FALSE; /* :-( */
	} else rtnval = FALSE;   /* :-( */
	return(rtnval);
}
/*
 *  char *_devtabpath()
 *
 *	Get the pathname of the device table
 *
 *  Arguments:  None
 *
 *  Returns:  char *
 *	Returns the pathname to the device table of (char *) NULL if
 *	there was a problem getting the memory needed to contain the
 *	pathname.
 *
 *  Algorithm:
 *	1.  If OAM_DEVTAB is defined in the environment and is not
 *	    defined as "", it returns the value of that environment
 *	    variable.
 *	2.  Otherwise, use the value of the environment variable DTAB_PATH.
 */


char *
_devtabpath()
{

	/* Automatic data */
#ifdef	DEBUG
	char	       *path;		/* Ptr to path in environment */
#endif
	char	       *rtnval;		/* Ptr to value to return */


	/*
	 * If compiled with -DDEBUG=1,
	 * look for the pathname in the environment
	 */

#ifdef	DEBUG
	if ((path = getenv(OAM_DEVTAB)) && (*path)) {
	    if (rtnval = (char *) malloc(strlen(path)+1)) (void) strcpy(rtnval, path);
	}
	else {
#endif
	    /*
	     * Use the standard device table.
	     */

	    if (rtnval = (char *) malloc(strlen(DTAB_PATH)+1)) (void) strcpy(rtnval, DTAB_PATH);

#ifdef	DEBUG
	}
#endif

	/* Finished */
	return(rtnval);
}

/* 
 * char *
 * getsecdev (char * tabentry)
 * returns the 4th entry after the first field 
 * in the string tabentry
 */
static char *
getsecdev(tabentry)
char * tabentry;
{
	int i;
	char *secdev;
	/* secdev is the fourth field after the alias */
	for (i=0;i<=3;i++)
		secdev = getfield(tabentry, ":", &tabentry);
	return(secdev);
}
void
_enddevtab() {}
