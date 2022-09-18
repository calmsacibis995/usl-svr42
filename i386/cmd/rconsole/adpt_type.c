/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rconsole:i386/cmd/rconsole/adpt_type.c	1.2"
#ident	"$Header: $"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <stdio.h>

extern int errno;
main( argc, argv )
int argc;
char **argv;
{
   extern int optind;
   extern char *optarg;
   int fd, exit_code, adp_type;
   struct kd_disparam kd_d; 
   struct kd_vdctype kd_v; 

   errno = 0;
   if ((fd=open("/dev/video", 2)) == -1 ) {
	   exit(0); /* non-integral console */
   }
   if (ioctl(fd,KDDISPTYPE, &kd_d)  == -1 ) {
      	    fprintf(stderr,"ioctl KDDSIPPARAM failed, errno %d\n", errno); 
	   exit(99); 	/* error */
   }
   switch (kd_d.type) {
	case KD_MONO:
		/* fprintf(stderr,"KD_MONO\n"); */
		exit_code=1;
		break;;
	case KD_EGA:
		/* fprintf(stderr,"KD_EGA\n"); */
		exit_code=9;
		break;;
	case KD_CGA:
		/* fprintf(stderr,"KD_CGA\n"); */
		exit_code=2;
		break;;
	case KD_VGA:
		/* fprintf(stderr,"KD_VGA\n"); */
                if (ioctl(fd,KDVDCTYPE, &kd_v)  == -1 ) {
      	           fprintf(stderr,"ioctl KDVDCTYPE failed, errno %d\n", errno); 
	           exit(98);
                }
                switch(kd_v.dsply ) {
                      case KD_MULTI_C:
                      case KD_STAND_C:
		           /*fprintf(stderr,"KD_MULTI_C\n"); */
		           exit_code=3;
		           break;;
                      case KD_MULTI_M:
                      case KD_STAND_M:
		           /* fprintf(stderr,"KD_MULTI_M\n"); */
		           exit_code=4;
		           break;;
	              default:
		           /* fprintf(stderr,"kd_v.dsply=%d\n",kd_v.dsply);*/
		           exit_code=5;
		           break;;
                }
		break;;
	default:
		/*fprintf(stderr,"adp_type=%d\n",kd_d.type);*/
		exit_code=10;
		break;;
	}
   exit(exit_code);
}
