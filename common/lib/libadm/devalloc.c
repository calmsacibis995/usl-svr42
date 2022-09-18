/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/devalloc.c	1.4.7.2"
#ident	"$Header: devalloc.c 1.3 91/06/25 $"

/*
 * Header Files :
 */
#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<devmgmt.h>
#include	<acl.h>
#include	<mac.h>
#include	"devtab.h"

/*
 * External References:
 */
void ddb_errmsg();


/*
 * Local Definitions:
 */
typedef struct devent {
	struct devent	*next;		/* ptr to next list-item    */
	char		*dsf;		/* pathname of dsf alloc'ed */
	level_t		level;		/* previous MAC level of dsf*/
	struct dev_daca	dsfdac;		/* previous DAC attrs of dsf*/
} dev_list;

static struct {
	dev_list	*list;
	int		cnt;
} dev_head = { (dev_list *)NULL, 0 };

static struct devstat	sysbuf = { DEV_SYSTEM, 0, 0, 0, 0, 0};

#define DEV_FREE	0	/* if use_count=0 & rel_flag=DEV_SYSTEM    */
#define DEV_ALLOCED	1	/* if use_count=0 & rel_flag=DEV_PERSISTENT*/
#define DEV_INUSE	2	/* if use_count=1.                         */

/* mode of allocated device special file */
#define DEVDACPERM	S_IRUSR|S_IWUSR	/* DAC mode = rw------- */


/*
 * int devalloc(device, cmd, bufp)
 * 	const char	*device;
 * 	int		cmd;
 * 	struct dev_alloca *bufp;
 *
 * This library functions allows processes to get or set security
 * attributes of the specified <device>, based on the specified <cmd>.
 * If <cmd> is DEV_GET, then it gets the security attributes
 * for the specified device, from the Device Database. 
 * If <cmd> is DEV_SET, then it sets the security attributes on the device 
 * special files mapped to the specified <device>.
 * 
 * <cmd> = DEV_GET: gets security attributes from the Device Database, 
 *                  in <bufp>. This function will not return any values 
 *                  for release flag or uid. No privileges are required
 *		    to access the Device Database.
 * 
 * <cmd> = DEV_SET: sets security attributes specified in <bufp>,
 *                  after validating the input attributes with the 
 *                  corresponding values specified in the DDB for that device.
 *		    The follwoing privileges are required to set secuirty
 *		    attributes on the device.
 * 
 *  PRIVILEGES REQUIRED:
 *	Inheritable:	P_DEV		for devstat(2)
 *			P_SETFLEVEL	for lvlfile(2)
 *			P_OWNER		for chown(2)
 *	Fixed      :	none
 *
 *  RETURN VALUE: int
 *
 *	0	if the function succeeds. 
 *	-1	if the function fails.
 *	   <errno> is set to one of the following values.
 *	   EACCES if access to Device Database is denied.
 *	   EACCES if Device Database files are not in consistent state.
 *	   EACCES if LTDB is not accessible or corrupted.
 *	   ENOENT if Device Database files cannot be found.
 *	   ENODEV if <device> is not defined in the Device Database
 *	   EINVAL if the specified <hilevel>  or <lolevel> or <level> is an 
 *		invalid level.
 *	   EINVAL if <cmd> is DEV_SET, if <hilevel> does not dominate <lolevel>.
 *	   EINVAL if <level> or level range specified is not enclosed by the 
 *		<range> stored in DDB for that device.
 *	   EINVAL if the specified <state> is not valid for the <device>.
 *	   EINVAL if the specified <mode> is not valid for the <device>.
 *	   EINVAL if invalid state is specified.
 *	   EINVAL if invalid mode is specified.
 *	   EINVAL if invalid command is specified.
 *	   EINVAL if invalid release flag is specified.
 *	   EINVAL if invalid uid is specified.
 *	   EINVAL if security attributes are not defined for the <device>.
 *	   EPERM if uid passed does not have authorization permission to have 
 *		a device allocated.
 *	   EBUSY if specified device (any dsf mapped to device) is not tranquil
 *	   EAGAIN if the Device Database is in Use, and cannot be locked.
 *	   ENOPKG if the enhanced security package is not installed.
 */
int 
devalloc(device, cmd, bufp)
 	char		*device;
 	int		cmd;
 	struct dev_alloca *bufp;
{
	char		*s_alias, 
			*ddb_range, *ddb_state, *ddb_mode, 
			*ddb_ual, *ddb_users, *ddb_other,
			*ddb_hi, *ddb_lo;
	level_t		ddbhigh, ddblow;
	int		state, mode, perm, dev_type;
	int		err, dacflag, rtn1, rtn2;
	struct passwd	*getpwuid();
	struct dev_daca	dacbuf;

	/* check if Device Database files are accessible and *
	 * in a consistent state - magic numbers match.      */

	if ((rtn1=ddb_check()) < 0) {
	    if (errno==0) {
		/* none of the DDB files present */
		errno = ENOENT;
	    }
	    return(FAILURE);
	} else if (rtn1 == 0) {
	    /* magic-nos in DDB files do not match */
	    errno = EACCES;
	    return(FAILURE);
	}

	if (cmd == DEV_GET) {
	    /* check if Enhanced Security package is installed */
	    if (!_mac_installed()) {
		/* errno set by lvlin(3) in _mac_installed */
		return(FAILURE);
	    }
	    if (s_alias = devattr(device, DDB_SECDEV)) {
		/* get essential security attrs of device */
		ddb_range = devattr(s_alias, DDB_RANGE);
		ddb_state = devattr(s_alias, DDB_STATE);
		ddb_mode  = devattr(s_alias, DDB_MODE);

		/* convert attributes from char. strings to binary values */
		if(ddb_range) {
		    /* extract hi-lo levels(LID strings from range */
		    ddb_hi = getfield(ddb_range,"-",&ddb_lo);
		    /* convert them to binary LID values */
		    bufp->hilevel = (level_t)strtol(ddb_hi,(char **)NULL,0);
		    bufp->lolevel = (level_t)strtol(ddb_lo,(char **)NULL,0);
		    /* levels (LIDs) not validated, because devalloc(DEV_GET) *
		     * must work during enhanced security installation, when  *
		     * MAC will be installed but NOT running.                 */
		} else {
		    /* error, device range not defined in DDB */
		    errno = EINVAL;
		    return(FAILURE);
		}
		if((bufp->state=parse_state(ddb_state))==0) {
		    /* error, device state in DDB is invalid */
		    errno = EINVAL;
		    return(FAILURE);
		}
		if((bufp->mode=parse_mode(ddb_mode))==0) {
		    /* error, device mode in DDB is invalid */
		    errno = EINVAL;
		    return(FAILURE);
		}
	    } else {
		/* devattr() function fails                  */
		/* NOTE: errno is set by devattr() function  *
		 * as follows :                              *
		 *	EACCES - if DDB cannot be accessed   *
		 *	ENOENT - if DDB files not present    *
		 *      ENODEV - if device not defined in DDB*/
		return(FAILURE);
	    }
	} else if (cmd == DEV_SET) {
	    /* check if Enhanced Security package is running */
	    if (!mac_running()) {
		/* errrno set by lvlproc(2) in mac_running */
		return(FAILURE);
	    }
	    /* validate input attrs in bufp */
	    if((!((bufp->state==DEV_PRIVATE)||(bufp->state==DEV_PUBLIC))) ||
	    (!((bufp->mode==DEV_STATIC)||(bufp->mode == DEV_DYNAMIC))) ||
	    (!((bufp->relflag==DEV_PERSISTENT)||(bufp->relflag==DEV_LASTCLOSE)))){
		errno = EINVAL;
		return(FAILURE);
	    }
	    /* check if input level is enclosed by input range */
	    if(((rtn1=lvldom(&bufp->hilevel,&bufp->level))<0)||
	      ((rtn2=lvldom(&bufp->level,&bufp->lolevel))<0)) {
		/* errno set to EINVAL by lvldom(2) */
		return(FAILURE);
	    } else if ((rtn1==0)||(rtn2==0)) {
		/* input device level is NOT enclosed by device range */
		errno = EINVAL;
		return(FAILURE);
	    }

	    /* lock the Device Database */
	    if (lock_ddb()==FAILURE) {
		/* error cannot lock DDB or access DDB files */
		errno = ddb_errget();
		return(FAILURE);
	    }

	    if (s_alias=devattr(device, DDB_SECDEV)) {
		if (strcmp(device,s_alias)==0) { 
		    dev_type = DEV_SECDEV;      /* secure  alias */
	        } else {
		    /* device <> s_alias */
		    if (valid_path(device))
			dev_type = DEV_DSF;      /* pathname of dsf */
		    else
			dev_type = DEV_ALIAS;    /* logical alias */
	        }
		/* get security attrs from Device Database */
		ddb_range = devattr(s_alias, DDB_RANGE);
		ddb_state = devattr(s_alias, DDB_STATE);
		ddb_mode = devattr(s_alias, DDB_MODE);
		ddb_ual = devattr(s_alias, DDB_UAL_ENABLE);
		ddb_users = devattr(s_alias, DDB_USERS);
		ddb_other = devattr(s_alias, DDB_OTHER);

		/* validate input attrs in bufp with attr-values from DDB */
		/* convert attributes from char. strings to binary values */
		if(ddb_range) {
		    /* extract hi-lo levels(LID strings from range */
		    ddb_hi = getfield(ddb_range,"-",&ddb_lo);
		    /* convert them to binary LID values */
		    ddbhigh = (level_t)strtol(ddb_hi,(char **)NULL,0);
		    ddblow = (level_t)strtol(ddb_lo,(char **)NULL,0);
		} else {
		    unlock_ddb();
		    errno = EINVAL;
		    return(FAILURE);
		} 
		/* check if bufp range is enclosed by DDB device range */
		if(((rtn1=lvldom(&ddbhigh,&bufp->hilevel))<0)||
	          ((rtn2=lvldom(&bufp->lolevel,&ddblow))<0)) {
		    /* error, invalid DDB device range, LTDB inaccessible */
		    err = errno;
		    unlock_ddb();
		    errno = err;
		    return(FAILURE);
		} else if ((rtn1==0)||(rtn2==0)) {
		    /* error, bufp range NOT enclosed by DDB device range */	
		    unlock_ddb();
		    errno = EINVAL;
		    return(FAILURE);
	        }

	        /* check if bufp state & mode are allowed by DDB  */
	        if ((bufp->state & parse_state(ddb_state))==0) {
		    unlock_ddb();
		    errno = EINVAL;
		    return(FAILURE);
		}

		if ((bufp->mode & parse_mode(ddb_mode))==0) {
		    unlock_ddb();
		    errno = EINVAL;
		    return(FAILURE);
		}

		/* check if user has authorization to access device */
		/* validate input uid */
		if (getpwuid(bufp->uid)==(struct passwd *)NULL) {
		    unlock_ddb();
		    errno = EINVAL;
		    return(FAILURE);
		}
		if((perm=devperm(bufp->uid,ddb_ual,ddb_users,ddb_other))
								==FALSE) {
		    unlock_ddb();
		    errno = EPERM;
		    return(FAILURE);
		}

		/* check if device (any dsf mapped to device) is "in use" */
		if ((rtn1=devinuse(s_alias, device))<0) {
		    /* error, devstat() failed on path mapped to s_alias */
		    err = errno;
		    unlock_ddb();
		    errno = err;
		    return(FAILURE);
		} else if (rtn1) {
		    /* error, device in use */
		    errno = EBUSY;
		    return(FAILURE);
		}

		/* setup the DAC attributes of the device being allocated */
		dacbuf.uid = bufp->uid;		/* input uid   */
		dacbuf.gid = -1;		/* gid not set */
		dacbuf.mode = DEVDACPERM;	/* mode_t      */

		/* Device not "in-use"; Set security attrs on device(dsfs)*/
		/* devsetdsfs() also un-does(back-tracks) upon failure    */
		if (devsetdsfs(device,dev_type,bufp,&dacbuf) < 0) {
		    err = errno;
		    unlock_ddb();
		    errno = err;
		    return(FAILURE);
		}
		unlock_ddb();
	    } else {
		/* devattr() function fails                  */
		/* NOTE: errno is set by devattr() function  *
		 * as follows :                              *
		 *	EACCES - if DDB cannot be accessed   *
		 *	ENOENT - if DDB files not present    *
		 *      ENODEV - if device not defined in DDB*/
		return(FAILURE);
	    }
	} else {
	    /* invalid <cmd> */
	    errno = EINVAL;
	    return(FAILURE);
	}
	return(SUCCESS);
}

/*
 * int devinuse(s_alias, device)
 * 	char	*s_alias;
 * 	char	*device;
 *
 * This function determines whether any of the device special files mapped
 * to the specified <s_alias> (secure device alias), are in use. 
 * If any one of them is in use, it returns a positive value,
 * otherwise it returns zero.
 * 
 * A device special file is considered to be "in use", if 
 *   - release_flag is set to DEV_PERSISTENT or DEV_LASTCLOSE.
 *   - there are any open connections (use_count=1).
 * This is determined by invoking the devstat() system call.
 *
 * Returns: int
 *	DEV_FREE(0)    if use_count=0 & relflag=DEV_SYSTEM.
 *	DEV_ALLOCED(1) if use_count=0 & relflag=DEV_PERSISTENT or DEV_LASTCLOSE.
 *	DEV_INUSE(2)   if use_count=1.
 *      -1             if devstat() fails, and sets <errno> and sets up
 *			the error message buffer with <device> it failed on.
 */
int 
devinuse(s_alias, device)
	char	*s_alias;
	char	*device;
{
	FILE	*fp;			/* DDB_DSFMAP file pointer */
	char	dsf[MAXDSFLEN];		/* buffer for next dsf     */
	struct devstat	bufp;		/* devstat buffer          */
	int	inuse, dsf_type, err;
	char	buf[80];	/* Where file's first line is stored.
				 * This value is not used */
	
	/* clear error code */
	ddb_errset(0);

	/* open DDB_DSFMAP file to get all dsfs mapped to s_alias */
	if ((fp=fopen(DDB_DSFMAP,"r"))==(FILE *)NULL) {
	     return(FAILURE);
	}

	/* skip magic number */
	if (fgets(buf,80,fp) == (char *)NULL) {
	    fclose(fp);
	    return(FAILURE);
	}

	inuse = DEV_FREE;
	/* get next dsf mapped to the secure device "s_alias" */
	while((getnextdsf(fp,s_alias,DEV_SECDEV,dsf,&dsf_type)==SUCCESS) &&
			(!inuse)) {
	    /* check if dsf is in use */
	    if (devstat(dsf, DEV_GET, &bufp) == 0) {
	        if (bufp.dev_relflag != DEV_SYSTEM)
	            inuse |= DEV_ALLOCED;
	        if (bufp.dev_usecount == 1)
	            inuse |= DEV_INUSE;
	    } else {
		/* errno set by devstat() system call */
		switch(err=errno) {
		case(ENODEV):
		    /* path not a block/char special file */
		    ddb_errmsg(SEV_ERROR, 4, E_NOTDSF, dsf, s_alias);
		    break;
		case(EPERM):
		    /* permission denied */
		    ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, 4, E_NOPATH, dsf, s_alias);
		    break;
		}
		errno = err;
	        return(FAILURE);
	    }
	} /* end while */
	/* check if there were any errors in querying DDB */
	if (err=ddb_errget()) {
	    /* error, close DDB_DSFMAP and return errno */
	    fclose(fp);
	    errno = err;
	    return(FAILURE);
	}
	fclose(fp);
	return(inuse);
}

/*
 * int devperm(uid, ual_enable, users, other)
 *	uid_t		uid;
 *	char		*ual_enable;
 *	char		*users;
 *	char		*other;
 *
 * The function determines if the specified user(<uid>) has
 * the permissions according to user authorization list, <users>,
 * and the authorization in other, provided the authorization list
 * is enabled (<ual_enable="y").
 *
 * The specified user has permissions to access device, if the
 * following conditions are all satisfied.
 * 1. The specified user (<uid)> occurs in <users> authorization list,
 *    and has permission turned on ("y"). The <users> list is assumed
 *    to contain uids in ascending order.
 * 2. If the user(<uid>) does not appear in the <user> authorization list,
 *    then the <other> permissions is looked at. If this attribute has 
 *    permission turned on ("y"), then user is granted permission
 *    to access the device.
 *
 * Returns: FILE *
 *    TRUE	if <uid> is granted permission to access the device
 *    FALSE	otherwise.
 */
int 
devperm(uid, ual_enable, users, other)
	uid_t		uid;
	char		*ual_enable;
	char		*users;
	char		*other;
{
	char		*ual;
	uid_t		ual_uid;
	char		*uperm, *next;
	char		*ual_user, *permstr;
	int		done;

	done = FALSE;
	/* check if user authorization list is enabled */
	if (*ual_enable != 'y') {
	    return(FALSE);
	}
	if (ual = users) {
	    /* check if user(uid) in <users> list */
	    while((!done)&&((uperm=(char *)getfield(ual,",",&next))!=NULL)) {
	        /* extract next user-perm pair from UAL */
	        ual_user = (char *)getfield(uperm,">",&permstr);
	        if ((ual_uid=(uid_t)atol(ual_user)) > uid) {
	    	    /* uid not found in UAL */
		    done = TRUE;
	    	    break;    /* break out of while */
	        } else if (ual_uid == uid) {
		    /* uid found in UAL; check alloc permission */
		    if (*(permstr)=='y')
	        	return(TRUE);     /* permitted to access */
		    else
	        	return(FALSE);    /* permission denied */
		}
		ual = next;
	    } /* end while */
	    if (!done) {
		/* extract last user-perm pair from UAL */
		ual_user = (char *)getfield(ual,">",&permstr);
		if ((ual_uid=(uid_t)atol(ual_user)) == uid) {
	            /* uid found in UAL; check alloc permission */
	            if (*(permstr)=='y')
	                return(TRUE);     /* permitted to access */
	            else
	                return(FALSE);    /* permission denied */
		}
	    }
	} /* end if */

	if ((other)&&(*(++other) == 'y')) {
	    return(TRUE);
	} else {
	    return(FALSE);
	}
}

/*
 * int devsetdsfs(device, dev_type, devbuf, dacbuf)
 * 	char			*device;
 * 	int			dev_type;
 * 	struct dev_alloca	*devbuf;
 * 	struct dev_daca		dacbuf;
 *
 * This functions sets the security attributes specified in <bufp>,
 * on the specified <device>. Where, <device> could be
 * either an absolute pathname to a character or block device special file(dsf),
 * or a <device alias> name.
 *
 * If specified <device> is a dsf(dev_type = DEV_DSF), then only that 
 * dsf is used to set security attributes. If <device> is a 
 * logical alias(dev_type = DEV_ALIAS), then all dsfs mapped to that 
 * alias, as defined in the DDB are used. If <device> is a 
 * secure device alias (dev_type = DEV_SECDEV), then
 * all dsfs mapped to the specified secure device alias are used.
 *
 * It uses the information in <dacbuf> to set the DAC ownership, group
 * and permissions on the device special files. If <uid> or <gid> specified
 * in <dacbuf> is -1, then the corresponding DAC permissions is not set.
 */
int 
devsetdsfs(device, dev_type, devbuf, dacbuf)
	char			*device;
	int			dev_type;
	struct dev_alloca	*devbuf;
	struct dev_daca		*dacbuf;
{
	FILE			*fp;
	uid_t			prev_uid;
	gid_t			prev_gid;
	level_t			prev_level;
	struct stat		statbuf;
	char 			buf[80];	/* Where file's first line is 
						 * read. Value is not used */

	/* default ACL for all devices; only the perms changed as per dacbuf */
	struct acl		aclbuf[NACLBASE] = {{USER_OBJ, 0, 0},
						    {GROUP_OBJ,0, 0},
						    {CLASS_OBJ,0, 0},
						    {OTHER_OBJ,0, 0}};

	dev_list		*prev_dev, *next_dev;
	char			dsf[MAXDSFLEN];
	char			alias[DDB_MAXALIAS],
				salias[DDB_MAXALIAS];
	int			dsftp;
	ushort			err;
	struct devstat		bufp;
	register int		i;

	/* clear error code */
	ddb_errset(0);
	err = FALSE;

	/* build struct devstat */
	bufp.dev_relflag = devbuf->relflag;
	bufp.dev_mode = devbuf->mode;
	bufp.dev_state = devbuf->state;
	bufp.dev_hilevel = devbuf->hilevel;
	bufp.dev_lolevel = devbuf->lolevel;
	/* if specified <device> = device special file */
	if (dev_type == DEV_DSF) {
	    /* get device alias the dsf maps to */
	    getdsfmap(device, &dsftp, alias, salias);

	    /* save current level in devlist */
	    lvlfile(device,MAC_GET,&prev_level);

	    /* save current DAC attrs of dsf */
	    if (stat(device, &statbuf) < 0) {
	        ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		return(FAILURE);
	    }
	
	    /* set level */
	    if (lvlfile(device,MAC_SET,&devbuf->level)<0) {
		switch(err=errno) {
		case(ENOSYS):
		    ddb_errmsg(SEV_ERROR, 4, E_NOLVL, device, alias);
		    break;
		case(EROFS):
		    ddb_errmsg(SEV_ERROR, 4, E_ROFS, device, alias);
		    break;
		case(EPERM):
		    ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    break;
		}
		return(FAILURE);
	    }

	    /* set DAC ownership */
	    if (chown(device,dacbuf->uid,dacbuf->gid)<0) {
		switch(err=errno) {
		case(EROFS):
		    ddb_errmsg(SEV_ERROR, 4, E_ROFS, device, alias);
		    break;
		case(EPERM):
		    ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    break;
		}
	        /* restore previous state of device */
	        lvlfile(device, MAC_SET, &prev_level);

	        devstat(device, DEV_SET, &sysbuf);

		errno = err;

	        return(FAILURE);
	    }

	    /* clear acl & set DAC permissions */
	    aclbuf[0].a_perm = (dacbuf->mode>>6) & 7;	/* USER_OBJ  */
	    aclbuf[1].a_perm = (dacbuf->mode>>3) & 7;	/* GROUP_OBJ */
	    aclbuf[2].a_perm = aclbuf[1].a_perm;	/* CLASS_OBJ */
	    aclbuf[3].a_perm = dacbuf->mode & 7;	/* OTHER_OBJ */
	    if (acl(device,ACL_SET,NACLBASE,aclbuf)<0) {
		err = errno;	/* save errno */
		switch(err=errno) {
		case(ENOSYS):
		    ddb_errmsg(SEV_ERROR, 4, E_NOACL, device, alias);
		    break;
		case(EPERM):
		    ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    break;
		}
		/* restore previous state of device */
		lvlfile(device, MAC_SET, &prev_level);

		chown(device,prev_uid,prev_gid);

		devstat(device, DEV_SET, &sysbuf);

	 	errno = err;	/*restore errno */
		return(FAILURE);
	    }
	    /* set security attrs on device */
	    if (devstat(device, DEV_SET, &bufp)<0) {
	        err = errno;
		switch(err=errno) {
		case(EPERM):
		    ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    break;
		}
	        /* set previous level */
	        lvlfile(device,MAC_SET,&prev_level);
	        chown(device,prev_uid,prev_gid);
		chmod(device, statbuf.st_mode);
		errno = err;	/*restore errno */
	        return(FAILURE);
	    }
	} else {
	    /* set sec. attrs. on all dsfs mapped to device */
	    if ((fp=fopen(DDB_DSFMAP,"r"))==(FILE *)NULL) {
		return(FAILURE);
	    }
	
	    /* skip magic no */
	    if (fgets(buf,80,fp) == (char *) NULL) {	
		fclose(fp);
		return(FAILURE);
	    }

	    ddb_errset(0);
	    if (getnextdsf(fp,device,dev_type,dsf,&dsftp)==SUCCESS) {
		/* allocate memory for next list item */
		if (prev_dev=(dev_list *)malloc(sizeof(dev_list))) {
		    dev_head.list = prev_dev;
		    dev_head.cnt++;
		    /* save dsf name in devlist */
		    if (prev_dev->dsf=(char *)malloc(strlen(dsf)+1)) {
			strcpy(prev_dev->dsf, dsf);
		    } else {
			return(FAILURE);
		    }
		} else return(FAILURE);
	    } else {

		/* error in accessing DDB_DSFMAP file */
		err = errno = ddb_errget();	

		/* If err is 0, then the alias probably
		 * didn't have any dsf mapped to it.
		 * Return SUCCESS, since it wasn't an error
		 */
		if (err)
			return(FAILURE);
		return(SUCCESS);
	    }
	
	    while (!err) {
		/* save current level in devlist */
		lvlfile(dsf,MAC_GET,&prev_dev->level);
		/* save current DAC attrs of dsf */
		if (stat(dsf, &statbuf) < 0) {
		    err = errno;
		    ddb_errmsg(SEV_ERROR, 4, E_PERM, device);
		    break;
		} else {
		    prev_dev->dsfdac.mode = statbuf.st_mode;
		    prev_dev->dsfdac.uid = statbuf.st_uid;
		    prev_dev->dsfdac.gid = statbuf.st_gid;
		}

		/* set level */
		if (lvlfile(dsf,MAC_SET,&devbuf->level)<0) {
		    switch(err=errno) {
		    case(ENOSYS):
			ddb_errmsg(SEV_ERROR, 4, E_NOLVL, dsf, device);
			break;
		    case(EROFS):
			ddb_errmsg(SEV_ERROR, 4, E_ROFS, dsf, device);
			break;
		    case(EPERM):
			ddb_errmsg(SEV_ERROR, 4, E_PERM, dsf);
			break;
		    default:
			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
			break;
		    }
		    break;
		}

	        /* set DAC ownership */
	        if (chown(dsf,dacbuf->uid,dacbuf->gid)<0) {
		    switch(err=errno) {
		    case(EPERM):
			ddb_errmsg(SEV_ERROR, 4, E_PERM, dsf);
			break;
		    default:
			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
			break;
		    }
	            /* restore previous state of device */
	            lvlfile(dsf, MAC_SET, &prev_dev->level);
	            devstat(dsf, DEV_SET, &sysbuf);
	            break;
	        }

		/* clear acl & set DAC permissions */
		aclbuf[0].a_perm = (dacbuf->mode>>6) & 7;	/* USER_OBJ  */
		aclbuf[1].a_perm = (dacbuf->mode>>3) & 7;	/* GROUP_OBJ */
		aclbuf[2].a_perm = aclbuf[1].a_perm;		/* CLASS_OBJ */
		aclbuf[3].a_perm = dacbuf->mode & 7;		/* OTHER_OBJ */
		if (acl(dsf,ACL_SET,NACLBASE,aclbuf)<0) {
		    switch(err=errno) {
		    case(ENOSYS):
		        ddb_errmsg(SEV_ERROR, 4, E_NOACL, dsf, device);
			break;
		    case(EPERM):
			ddb_errmsg(SEV_ERROR, 4, E_PERM, dsf);
			break;
		    default:
			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
			break;
		    }
		    /* restore previous state of device */
		    lvlfile(dsf, MAC_SET, &prev_dev->level);
		    chown(dsf,prev_dev->dsfdac.uid,prev_dev->dsfdac.gid);
		    devstat(dsf, DEV_SET, &sysbuf);
		    break;
		}
		if (devstat (dsf, DEV_SET, &bufp)<0) {
		    switch(err=errno) {
		    case(EPERM):
			ddb_errmsg(SEV_ERROR, 4, E_PERM, dsf);
			break;
		    default:
			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
			break;
		    }
		    /* set previous level */
		    lvlfile(dsf,MAC_SET,&prev_dev->level);
		    chown(dsf,prev_dev->dsfdac.uid,prev_dev->dsfdac.gid);
		    chmod(dsf, prev_dev->dsfdac.mode);
		    break;
		}
		if (getnextdsf(fp,device,dev_type,dsf,&dsftp)==SUCCESS) {
		    /* allocate memory for next list item */
		    if (next_dev=malloc(sizeof(dev_list))) {
			prev_dev->next = next_dev;
			prev_dev = next_dev;
			dev_head.cnt++;
			/* save dsf name in devlist */
			if (prev_dev->dsf=malloc(strlen(dsf)+1)) {
			    strcpy(prev_dev->dsf, dsf);
			} else {
			    err = errno;
			    break;
			}
		    } else {
			err = errno;
			break;
		    }
		} else {
		    err = ddb_errget();	
		    break;
		}
	    }	/* end while */
	    /* if error undo work done on allocated dsfs */
	    if (err) {
		prev_dev = dev_head.list;
		dev_head.cnt--;		/* last failed device already reset */
		for (i=1; i<=dev_head.cnt; prev_dev=prev_dev->next) {
		    /* restore previous sec attrs of device */
		    lvlfile(dsf, MAC_SET, &prev_dev->level);
		    devstat(dsf, DEV_SET, &sysbuf);
		    /* restore previous DAC attrs of device */
	    	    chown(dsf,prev_dev->dsfdac.uid,prev_dev->dsfdac.gid);
	    	    chmod(dsf,prev_dev->dsfdac.mode);
		}	/* end for loop */
		fclose(fp);
		errno = err;	/* restore errno */
		return(FAILURE);  
	    }
	    fclose(fp);
	}
	return(SUCCESS);
}
