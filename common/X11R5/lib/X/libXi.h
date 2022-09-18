/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)R5Xlib:libXi.h	1.1"
#endif

#ifdef SHARELIB
#define fscanf	(*_libX_fscanf)
#define sscanf	(*_libX_sscanf)
#define fgets	(*_libX_fgets)
#define fopen	(*_libX_fopen)
#define fclose	(*_libX_fclose)
#define abs	(*_libX_abs)
#define fwrite	(*_libX_fwrite)
#define fputs	(*_libX_fputs)
#define getpwuid	(*_libX_getpwuid)
#define getuid	(*_libX_getuid)
#define getpwnam	(*_libX_getpwnam)
#define getenv	(*_libX_getenv)
#define ioctl	(*_libX_ioctl)
#define strcpy	(*_libX_strcpy)
#define fcntl	(*_libX_fcntl)
#define _ctype	(*_libX__ctype)
#define memchr	(*_libX_memchr)
#define memccpy	(*_libX_memccpy)
#define strcmp	(*_libX_strcmp)
#define exit	(*_libX_exit)
#define realloc	(*_libX_realloc)
#define calloc	(*_libX_calloc)
#define t_unbind	(*_libX_t_unbind)
#define t_snddis	(*_libX_t_snddis)
#define t_snd	(*_libX_t_snd)
#define t_rcv	(*_libX_t_rcv)
#define t_connect	(*_libX_t_connect)
#define t_free	(*_libX_t_free)
#define t_accept	(*_libX_t_accept)
#define t_close	(*_libX_t_close)
#define t_errno	(*_libX_t_errno)
#define t_listen	(*_libX_t_listen)
#define t_look	(*_libX_t_look)
#define t_bind	(*_libX_t_bind)
#define t_alloc	(*_libX_t_alloc)
#define t_error	(*_libX_t_error)
#define t_open	(*_libX_t_open)
#define alarm	(*_libX_alarm)
#define signal	(*_libX_signal)
#define strlen	(*_libX_strlen)
#define write	(*_libX_write)
#define strcat	(*_libX_strcat)
#define read	(*_libX_read)
#define close	(*_libX_close)
#define chmod	(*_libX_chmod)
#define link	(*_libX_link)
#define ptsname	(*_libX_ptsname)
#define unlink	(*_libX_unlink)
#define sprintf	(*_libX_sprintf)
#define atoi	(*_libX_atoi)
#define unlockpt	(*_libX_unlockpt)
#define grantpt	(*_libX_grantpt)
#define perror	(*_libX_perror)
#define open	(*_libX_open)
#define fprintf	(*_libX_fprintf)
#define _iob	(*_libX__iob)
#define poll	(*_libX_poll)
#define ulimit	(*_libX_ulimit)
#define srand	(*_libX_srand)
#define rand	(*_libX_rand)
#define free	(*_libX_free)
#define malloc	(*_libX_malloc)
#define strrchr	(*_libX_strrchr)
#define strchr	(*_libX_strchr)
#define daylight	(*_libX_daylight)
#define timezone	(*_libX_timezone)
#define strncpy	(*_libX_strncpy)
#define uname	(*_libX_uname)
#define errno	(*_libX_errno)
#define memset	(*_libX_memset)
#define memcmp	(*_libX_memcmp)
#define memcpy	(*_libX_memcpy)

extern int        fscanf();
extern int        sscanf();
extern char *     fgets();
extern int        fclose();
extern int        abs();
extern int        fwrite();
extern int        fputs();
extern unsigned short getuid();
extern char *     getenv();
extern int        ioctl();
extern char *     strcpy();
extern int        fcntl();
extern char *     memchr();
extern char *     memccpy();
extern int        strcmp();
extern void       exit();
extern char *     realloc();
extern char *     calloc();
extern int        t_unbind();
extern int        t_snddis();
extern int        t_snd();
extern int        t_rcv();
extern int        t_connect();
extern int        t_free();
extern int        t_accept();
extern int        t_close();
extern int        t_errno;
extern int        t_listen();
extern int        t_look();
extern int        t_bind();
extern char *     t_alloc();
extern int        t_error();
extern int        t_open();
extern unsigned   alarm();
extern int        strlen();
extern int        write();
extern char *     strcat();
extern int        read();
extern int        close();
extern int        chmod();
extern int        link();
extern char *     ptsname();
extern int        unlink();
extern int        sprintf();
extern int        atoi();
extern void       unlockpt();
extern void       grantpt();
extern void       perror();
extern int        open();
extern int        fprintf();
extern int        poll();
extern long       ulimit();
extern void       srand();
extern int        rand();
extern void       free();
extern char *     malloc();
extern char *     strrchr();
extern char *     strchr();
extern int        daylight;
extern long       timezone;
extern char *     strncpy();
extern int        uname();
extern int        errno;
extern char *     memset();
extern int        memcmp();
extern char *     memcpy();
#endif
