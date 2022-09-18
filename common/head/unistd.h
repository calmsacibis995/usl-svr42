/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UNISTD_H
#define _UNISTD_H

#ident	"@(#)sgs-head:unistd.h	1.39"

#ifndef _SSIZE_T
#define _SSIZE_T
typedef int     ssize_t; /* to return byte count or indicate error */
#endif

/* Symbolic constants for the "access" routine: */
#define	R_OK	004	/* Test for Read permission */
#define	W_OK	002	/* Test for Write permission */
#define	X_OK	001	/* Test for eXecute permission */
#define	F_OK	000	/* Test for existence of File */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define EFF_ONLY_OK 	010	/* Test using effective ids */
#define EX_OK		020	/* Test for Regular, executable file */
#endif

#if !defined(_POSIX_SOURCE) 
#define F_ULOCK	0	/* Unlock a previously locked region */
#define F_LOCK	1	/* Lock a region for exclusive use */
#define F_TLOCK	2	/* Test and lock a region for exclusive use */
#define F_TEST	3	/* Test a region for other processes locks */
#endif /* !defined(_POSIX_SOURCE) */ 


/* Symbolic constants for the "lseek" routine: */
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */

#if !defined(_POSIX_SOURCE) 
/* Path names: */
#define	GF_PATH	"/etc/group"	/* Path name of the "group" file */
#define	PF_PATH	"/etc/passwd"	/* Path name of the "passwd" file */
#endif /* !defined(_POSIX_SOURCE) */ 

#include <sys/unistd.h>


/* compile-time symbolic constants,
** Support does not mean the feature is enabled.
** Use pathconf/sysconf to obtain actual configuration value.
** 
*/

#define _POSIX_JOB_CONTROL	1
#define _POSIX_SAVED_IDS	1

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE		0
#endif

#ifndef	NULL
#define NULL	0
#endif

#define	STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/* Current version of POSIX */
#ifndef _POSIX_VERSION
#define _POSIX_VERSION		198808L
#endif

/* Current version of XOPEN */
#define _XOPEN_VERSION	3

#if defined(__STDC__)

#include <sys/types.h>

extern int access(const char *, int);
#if !defined(_POSIX_SOURCE) 
extern int acct(const char *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern unsigned alarm(unsigned);
#if !defined(_POSIX_SOURCE) 
extern int brk(void *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern int chdir(const char *);
extern int chown(const char *, uid_t, gid_t);
#if !defined(_POSIX_SOURCE) 
extern int chroot(const char *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern int close(int);
#if !defined(_POSIX_SOURCE)
extern char *ctermid(char *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern char *cuserid(char *);
extern int dup(int);
extern int dup2(int, int);
extern int execl(const char *, const char *, ...);
extern int execle(const char *, const char *, ...);
extern int execlp(const char *, const char *, ...);
extern int execv(const char *, char *const *);
extern int execve(const char *, char *const *, char *const *);
extern int execvp(const char *, char *const *);
#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
extern void exit(int);
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */
extern void _exit(int);
#if !defined(_POSIX_SOURCE)
extern int fattach(int, const char *);
extern int fchdir(int);
extern int fchown(int,uid_t, gid_t);
extern int fdetach(const char *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern pid_t fork(void);
extern long fpathconf(int, int);
#if !defined(_POSIX_SOURCE)
extern int fsync(int);
extern int ftruncate(int, off_t);
#endif /* !defined(_POSIX_SOURCE) */ 
extern char *getcwd(char *, size_t);
extern gid_t getegid(void);
extern uid_t geteuid(void);
extern gid_t getgid(void);
extern int getgroups(int, gid_t *);
extern char *getlogin(void);
#if !defined(_POSIX_SOURCE)
extern pid_t getpgid(pid_t);
#endif /* !defined(_POSIX_SOURCE) */ 
extern pid_t getpid(void);
extern pid_t getppid(void);
extern pid_t getpgrp(void);
#if !defined(_POSIX_SOURCE)
char *gettxt(const char *, const char *);
extern pid_t getsid(pid_t);
#endif /* !defined(_POSIX_SOURCE) */ 
extern uid_t getuid(void);
#if !defined(_POSIX_SOURCE) 
extern int ioctl(int, int, ...);
#endif /* !defined(_POSIX_SOURCE) */ 
extern int isatty(int);
extern int link(const char *, const char *);
#if !defined(_POSIX_SOURCE) 
extern int lchown(const char *, uid_t, gid_t);
extern int lockf(int, int, long);
#endif /* !defined(_POSIX_SOURCE) */ 
extern off_t lseek(int, off_t, int);
#if !defined(_POSIX_SOURCE) 
extern int mincore(caddr_t, size_t, char *);
extern int nice(int);
#endif /* !defined(_POSIX_SOURCE) */ 
extern long pathconf(const char *, int);
extern int pause(void);
extern int pipe(int *);
#if !defined(_POSIX_SOURCE) 
extern void profil(unsigned short *, unsigned int, unsigned int, unsigned int);
extern int ptrace(int, pid_t, int, int);
#endif /* !defined(_POSIX_SOURCE) */ 
extern ssize_t read(int, void *, size_t);
#if !defined(_POSIX_SOURCE) 
extern int readlink(const char *, void *, int);
extern int rename(const char *, const char *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern int rmdir(const char *);
#if !defined(_POSIX_SOURCE) 
extern void *sbrk(int);
#endif /* !defined(_POSIX_SOURCE) */ 
extern int setgid(gid_t);
#if !defined(_POSIX_SOURCE) 
extern int setgroups(int, const gid_t *);
#endif /* !defined(_POSIX_SOURCE) */ 
extern int setpgid(pid_t, pid_t);
#if !defined(_POSIX_SOURCE) 
extern pid_t setpgrp(void);
#endif /* !defined(_POSIX_SOURCE) */ 
extern pid_t setsid(void);
extern int setuid(uid_t);
extern unsigned sleep(unsigned);
#if !defined(_POSIX_SOURCE) 
extern int stime(const time_t *);
extern int symlink(const char *, const char *);
extern void sync(void);
#endif /* !defined(_POSIX_SOURCE) */ 
extern long sysconf(int);
extern pid_t tcgetpgrp(int);
extern int tcsetpgrp(int, pid_t);
#if !defined(_POSIX_SOURCE) 
extern int truncate(const char *, off_t);
#endif /* !defined(_POSIX_SOURCE) */ 
extern char *ttyname(int);
extern int unlink(const char *);
#if !defined(_POSIX_SOURCE)
extern pid_t vfork(void);
#endif /* !defined(_POSIX_SOURCE) */ 
extern ssize_t write(int, const void *, size_t);

#else       /*  STDC not defined  */

extern int access();
#if !defined(_POSIX_SOURCE) 
extern int acct();
#endif /* !defined(_POSIX_SOURCE) */ 
extern unsigned alarm();
#if !defined(_POSIX_SOURCE) 
extern int brk();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int chdir();
extern int chown();
#if !defined(_POSIX_SOURCE) 
extern int chroot();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int close();
#if !defined(_POSIX_SOURCE)
extern char *ctermid();
#endif /* !defined(_POSIX_SOURCE) */ 
extern char *cuserid();
extern int dup();
extern int dup2();
extern int execl();
extern int execle();
extern int execlp();
extern int execv();
extern int execve();
extern int execvp();
#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
extern void exit();
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */
extern void _exit();
#if !defined(_POSIX_SOURCE)
extern int fattach();
extern int fchdir();
extern int fchown();
extern int fdetach();
#endif /* !defined(_POSIX_SOURCE) */ 
extern long fpathconf();
#if !defined(_POSIX_SOURCE)
extern int fsync();
extern int ftruncate();
#endif /* !defined(_POSIX_SOURCE) */ 
extern char *getcwd();
extern int getgid();
extern int getgroups();
extern char *getlogin();
#if !defined(_POSIX_SOURCE)
extern int getpgid();
extern int getsid();
char *gettxt();
extern int ioctl();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int isatty();
#if !defined(_POSIX_SOURCE) 
extern int lchown();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int link();
#if !defined(_POSIX_SOURCE) 
extern int lockf();
extern int mincore();
extern int nice();
#endif /* !defined(_POSIX_SOURCE) */ 
extern long pathconf();
extern int pause();
extern int pipe();
#if !defined(_POSIX_SOURCE) 
extern void profil();
extern int ptrace();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int read();
#if !defined(_POSIX_SOURCE) 
extern int readlink();
extern int rename();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int rmdir();
#if !defined(_POSIX_SOURCE) 
extern char *sbrk();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int setgid();
#if !defined(_POSIX_SOURCE) 
extern int setgroups();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int setpgid();
extern int setuid();
extern unsigned sleep();
#if !defined(_POSIX_SOURCE) 
extern int stime();
extern int symlink();
extern void sync();
#endif /* !defined(_POSIX_SOURCE) */ 
extern long sysconf();
extern int tcsetpgrp();
#if !defined(_POSIX_SOURCE)
extern int truncate();
#endif /* !defined(_POSIX_SOURCE) */ 
extern char *ttyname();
extern int unlink();
#if !defined(_POSIX_SOURCE)
extern int vfork();
#endif /* !defined(_POSIX_SOURCE) */ 
extern int write();

#if !defined(_POSIX_SOURCE)
extern int fork();
extern int getegid();
extern int geteuid();
extern int getgid();
extern int getpid();
extern int getppid();
extern int getpgrp();
extern int getuid();
extern long lseek();
extern int setpgrp();
extern int setsid();
extern int tcgetpgrp();
#else
extern pid_t fork();
extern gid_t getegid();
extern uid_t geteuid();
extern gid_t getgid();
extern pid_t getpid();
extern pid_t getppid();
extern pid_t getpgrp();
extern uid_t getuid();
extern off_t lseek();
extern pid_t setpgrp();
extern pid_t setsid();
extern pid_t tcgetpgrp();
#endif /* !defined(_POSIX_SOURCE) */

#endif

#endif /* _UNISTD_H */
