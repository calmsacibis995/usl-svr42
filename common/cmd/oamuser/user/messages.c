/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/messages.c	1.6.12.4"
#ident  "$Header: messages.c 2.0 91/07/13 $"

char *errmsgs[] = {
	":0:uid %ld is reserved.\n",
	":0:more than NGROUPS_MAX(%d) groups specified.\n",
	":0:invalid syntax.\nusage: useradd [-u uid [-o] [-i] | -g group | -G group[[,group]...] | -d dir |\n               -s shell | -c comment | -m [-k skel_dir] | -f inactive |\n               -e expire | -p passgen%s%s] login\n",
	":0:Invalid syntax.\nusage:  userdel [-r] [-n months] login\n",
	":0:invalid syntax.\nusage: usermod [-u uid [-o] | -g group | -G group[[,group]...] | -d dir [-m] |\n               -s shell | -c comment | -l new_logname | -f inactive |\n               -e expire | -p passgen%s%s] login\n",
	":0:Unexpected failure.  Defaults unchanged.\n",
	":0:Unable to remove files from home directory.\n",
	":0:Unable to remove home directory.\n",
	":0:Cannot update system files - login cannot be %s.\n",
	":0:uid %ld is already in use.  Choose another.\n",
	":0:%s is already in use.  Choose another.\n",
	":0:%s does not exist.\n",
	":0:%s is not a valid %s.  Choose another.\n",
	":0:%s is in use.  Cannot %s it.\n",
	":0:%s has no permissions to use %s.\n",
	":0:There is not sufficient space to move %s home directory to %s\n",
	":0:%s %s is too big.  Choose another.\n",
	":0:group %s does not exist.  Choose another.\n",
	":0:Unable to %s: %s.\n",
	":0:%s is not a full path name.  Choose another.\n",
	":0:invalid argument specified with -p flag\n",
	":0:invalid audit event type or class specified.\n",
	":0:invalid security level specified.\n",
	":0:invalid default security level specified.\n",
	":0:invalid option -a\n",
	":0:invalid options -h\n",
	":0:system service not installed.\n",
	":0:cannot delete security level %s.\n                Current default security level will become invalid.\n",
	":0:Invalid syntax.\nusage:  usermod -u uid [-o] | -g group | -G group[[,group]...] |\n                -d dir [-m] | -s shell | -c comment |\n                -l new_logname | -f inactive | -e expire\n                -h [operator]level[-h level] | -v def_level |\n                -a[operator]event[,..]   login\n",
	":0:%s is the primary group name.  Choose another.\n",
	":0:invalid security level specified for user's home directory.\n",
	":0:user's home directory already exists, -w ignored.\n",
	":0:invalid months value specified for uid aging.\n",
	":0:uid %d not aged sufficiently. Choose another.\n",
	":0:unable to access ``%s''\n",
	":0:The DATEMSK environment variable is null or undefined.\n",
	":0:The /etc/datemsk file cannot be opened for reading.\n",
	":0:Failed to get /etc/datemsk file status information.\n",
	":0:The /etc/datemsk file is not a regular file.\n",
	":0:An error is encountered while reading the /etc/datemsk file.\n",
	":0:malloc failed (not enough memory is available).\n",
	":0:login name ``%s'' may produce unexepected results when used with other commands on the system\n",
	":0:invalid options -v\n",
	":0:invalid options -w\n"
};

int lasterrmsg = sizeof( errmsgs ) / sizeof( char * );
