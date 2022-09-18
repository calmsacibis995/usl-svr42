/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/ipc/sem.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/map.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define max(a,b) ((a)>(b)?(a):(b)) 

struct	semid_ds	sema[SEMMNI] ;
struct	sem	sem[SEMMNS] ;
struct	map	semmap[SEMMAP] ;
long		sem_undo[NPROC] ;
int		semu[((16+8*SEMUME)*SEMMNU+NBPW-1)/NBPW] ;

int		semtmp[(max(2*SEMMSL,max(0x58,8*SEMOPM))+NBPW-1)/NBPW] ;
struct	seminfo	seminfo
		      ={SEMMAP,
			SEMMNI,
			SEMMNS,
			SEMMNU,
			SEMMSL,
			SEMOPM,
			SEMUME,
			16+8*SEMUME,
			SEMVMX,
			SEMAEM} ;
