/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/messages.h	1.4.11.4"
#ident  "$Header: messages.h 2.0 91/07/13 $"
/* Error returned by getdate(3C)	*/
#define GETDATE_ERR		7

/* WARNING: uid %d is reserved. */
#define M_RESERVED		0

/* WARNING: more than NGROUPS_MAX(%d) groups specified. */
#define M_MAXGROUPS	1

/* ERROR:invalid syntax.\nusage: useradd [-u uid [-o] [-i] | -g group | -G group[[,group]...] | -d dir |\n               -s shell | -c comment | -m [-k skel_dir] | -f inactive |\n               -e expire | -p passgen%s%s] login\n */
#define M_AUSAGE		2

/* ERROR: Invalid syntax.\nusage:  userdel [-r] [-n months] login */
#define M_DUSAGE		3

/* ERROR: Invalid syntax.\nusage:  usermod -u uid [-o] | -g group | -G group[[,group]...] | -d dir [-m] |\n                -s shell | -c comment | -l new_logname login */
#define M_MUSAGE		4

/* ERROR: Unexpected failure.  Defaults unchanged. */
#define M_FAILED	5

/* ERROR: Unable to remove files from home directory. */
#define M_RMFILES	6

/* ERROR: Unable to remove home directory. */
#define M_RMHOME		7

/* ERROR: Cannot update system files - login cannot be %s. */
#define M_UPDATE		8

/* ERROR: uid %d is already in use.  Choose another. */
#define M_UID_USED	9

/* ERROR: %s is already in use.  Choose another. */
#define M_USED	10

/* ERROR: %s does not exist. */
#define M_EXIST	11

/* ERROR: %s is not a valid %s.  Choose another. */
#define M_INVALID		12

/* ERROR: %s is in use.  Cannot %s it. */
#define M_BUSY	13

/* WARNING: %s has no permissions to use %s. */
#define M_NO_PERM	14

/* ERROR: There is not sufficient space to move %s home directory to %s */
#define M_NOSPACE		15

/* ERROR: %s %d is too big.  Choose another. */
#define	M_TOOBIG	16

/* ERROR: group %s does not exist.  Choose another. */
#define	M_GRP_NOTUSED	17

/* ERROR: Unable to %s: %s */
#define	M_OOPS	18

/* ERROR: %s is not a full path name.  Choose another. */
#define	M_RELPATH	19

/* "ERROR: invalid argument specified with -p flag\n" */
#define M_BAD_PFLG	20

/* "ERROR: invalid audit event type or class specified.\n"	*/
#define	M_BADMASK	21

/* "ERROR: invalid security level specified.\n", */
#define	M_INVALID_LVL	22

/* "ERROR: invalid default security level specified.\n", */
#define	M_INVALID_DLVL	23

/* "ERROR: invalid option -a\n", */
#define	M_NO_AUDIT	24

/* "ERROR: invalid options -h\n", */
#define	M_NO_MAC_H	25

/* "ERROR: system service not installed.\n", */
#define	M_NO_SERVICE	26

/* "ERROR: cannot delete security level %s.\n                Current default security level will become invalid.\n"
*/
#define	M_NO_DEFLVL	27
/* "ERROR: Invalid syntax.\nusage:  usermod -u uid [-o] | -g group | -G group[[,group]...] |\n                -d dir [-m] | -s shell | -c comment |\n
          -l new_logname | -f inactive | -e expire\n               -h [operator]
level[,..] | -v def_level | -a[operator]event[,..]   login\n"    */

#define	M_MUSAGE1	28

/* "ERROR: %s is the primary group name.  Choose another.\n" */
#define	M_SAME_GRP	29

/* "ERROR: invalid security level specified for user's home directory.\n", */
#define	M_INVALID_WLVL	30

/* "WARN: user's home directory already exists, -w ignored.\n", */
#define M_INVALID_WOPT  31

/* "ERROR: invalid months value specified for uid aging.\n", */
#define M_INVALID_AGE  	32

/* "ERROR: uid %d not aged sufficiently. Choose another.\n" */
#define M_UID_AGED	33

/* "ERROR: unable to access ``%s''\n"	*/
#define M_NOACCESS	34

/* "ERROR: The DATEMSK environment variable is null or undefined.\n" */
#define M_GETDATE	35

/* Error numbers 36-40 are reserved for getdate(3C) errors. 	*/

/* "WARNING: A logname of %s can cause produce unexepected results when used with other commands on the system\n" */
#define	M_NUMERIC	41

/* "ERROR: invalid options -v\n", */
#define	M_NO_MAC_V	42

/* "ERROR: invalid options -w\n", */
#define	M_NO_MAC_W	43
