/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)wsinit:wsinit.c	1.7"

#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/stropts.h>
#include	<sys/fcntl.h>
#include	<sys/mkdev.h> 
#include	<sys/genvid.h>
#include	<grp.h>
#include	<malloc.h>
#include	<ftw.h>

char	*WSFILEDEFAULT	= "/dev/vt	/dev/kd/kd	9";
char	*WSFILE		= "/etc/default/workstations";
char	*WSDOTFILE	= "/etc/.wsinitdate";
char	*GVIDFILE	= "/dev/vidadm";
#define SYSNCHAN	15
#define	MAXLEN	1024
#define	CMUX_MAJOR	5


#ifndef _STYPES
#define SYSMAXMINOR	MAXMIN
#else
#define	SYSMAXMINOR	OMAXMIN
#endif

struct group *getgrnam();

int cmux_minor = 0;
gvid_t Gvid;
dev_t *devbuf;
int devbufsize = 0;
struct group *grp;
int remake_devs = 0;

void
alloc_devbuf(size)
unsigned size;
{
	if (size <= devbufsize || size == 0)
		return;

	if (devbuf == (dev_t *) NULL) 
		devbuf = (dev_t *) malloc(size*sizeof(dev_t));
	else
		devbuf = (dev_t *)realloc((char *)devbuf,size*sizeof(dev_t));

	if (devbuf == (dev_t *) NULL) {
		disaster(10,"out of space");
		return;
	}
	devbufsize = size;
	return;
}

int
need_to_update_devs()
{
	struct stat wsfile_time;
	struct stat dotfile_time;

	if (stat(WSFILE,&wsfile_time) < 0) {
		perror("stat of workstations file:");
		exit(1);
	}
	if (stat(WSDOTFILE,&dotfile_time) < 0) 
		return (1); /* no dot file; we need to update devs */

	if (dotfile_time.st_mtime <= wsfile_time.st_mtime)
		return (1); /* ws file later than dot file; update devs */

	return (0); /* don't need to update devs */
}


main(argc, argv)
int argc;
char **argv;
{
	FILE *fp;
	struct	stat wsstatbuf;
	int dotfilefd;
	char chan[MAXLEN],driver[MAXLEN],buf[3*MAXLEN];
	int gvidfd;
	int max, i;
	int chanmaj, drvmaj;
	
	devbuf = (dev_t *) NULL;

	grp = getgrnam("tty");
	if (grp == (struct group *) NULL)
		disaster(8,"tty");

	alloc_devbuf(256); 

	if (devbuf != (dev_t *) NULL)
		for (i=0; i<devbufsize; i++)
			*(devbuf +i) = NODEV;

	chanmaj = CMUX_MAJOR; 

	if ( stat(WSFILE, &wsstatbuf) == -1 ) {
		int	fd;
		char	buf[128];
		
		sprintf(buf, "wsinit: Cannot stat %s file\n", WSFILE);
		perror(buf);
		mkdir("/etc/default", 0x755);
 		fd = creat(WSFILE, 0444);
		if(fd < 0) {
			sprintf(buf, "wsinit: Cannot create %s file\n", WSFILE);
			perror(buf);
			exit(1);
		}
		else {
			write(fd, WSFILEDEFAULT, strlen(WSFILEDEFAULT));
			write(fd, "\n", 1);
			close(fd);
		}
		stat(WSFILE, &wsstatbuf);
		remake_devs = 1;
	}

	if ( (fp=fopen(WSFILE, "r")) == NULL ) {
		perror("fopen:");
		disaster(3, WSFILE);
		exit(1);
	}

	if (wsstatbuf.st_size == 0) {
		disaster(12, WSFILE);
		exit(1);
	}
		
	if(!remake_devs)
		remake_devs = need_to_update_devs();

	while ( fgets(buf,3*MAXLEN -1,fp) != (char *) NULL) {
		if (buf[0] == '#')
			continue;
		if(sscanf(buf,"%s %s %d",chan, driver, &max) != 3)
				continue;
		if (cmux_minor % SYSNCHAN)
			cmux_minor += SYSNCHAN - (cmux_minor % SYSNCHAN);
		if (cmux_minor >= devbufsize)
			alloc_devbuf(devbufsize*2);
		if (cmux_minor > SYSMAXMINOR) {
			disaster(7,chan);
			exit(7);
		}

		/* constrain max to values <= SYSNCHAN */
		max = (max > SYSNCHAN) ? SYSNCHAN : max;

		mkchan(chan,chanmaj,max,driver);
	}

	if (remake_devs) {
	   /* create/overwrite dot file and set mod time on dot file */
	   dotfilefd = creat(WSDOTFILE, 0777777);
	   close (dotfilefd);
	}

	if ( (gvidfd=open(GVIDFILE, O_RDWR)) == -1) {
		perror("open:");
		disaster(3,GVIDFILE);
		exit(3);
	}
	
	if (devbuf != (dev_t *) NULL) {
		Gvid.gvid_num = cmux_minor;
		Gvid.gvid_maj = chanmaj;
		Gvid.gvid_buf = &devbuf[0];
		if (ioctl(gvidfd,GVID_SETTABLE,&Gvid) < 0) {
			perror("ioctl");
			disaster(8,GVIDFILE);
			exit(8);
		}
	}

	exit (0);
}

int
mkchan(chan,maj,max,drv)
char *chan, *drv;
int maj,max;
{
	register int i;		
	register int dev;		
	char *chantmp,*drvtmp,*vidtmp;
	int chanlen=strlen(chan);
	int drvlen=strlen(drv);
	int muxfd, devfd;
	struct stat vidstatbuf;
	int mode,gid;

	chantmp = (char *)malloc(chanlen+3);
	drvtmp = (char *)malloc(drvlen+3);
	vidtmp = (char *)malloc(drvlen+5);

	strcpy(chantmp,chan);
	strcpy(drvtmp,drv);
	strcpy(vidtmp,drv);

	mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH;

	if (grp == (struct group *) NULL)
		gid = 0;
	else
		gid = grp->gr_gid;

	if (remake_devs) {
	   for (i=0; i < SYSNCHAN; i++) {
		sprintf(chantmp,"%s%.2d",chan,i);
		if (access(chantmp,0) != -1) {
			if ( unlink(chantmp) == -1 ) 
				disaster(1,chantmp);
		}
	   }
	}


	for (i=0;i < max;i++) {
		sprintf(chantmp,"%s%.2d",chan,i);
		sprintf(vidtmp,"%svm%.2d",drv,i);
		sprintf(drvtmp,"%s%.2d",drv,i);

		dev = makedev(maj, cmux_minor);
		if (remake_devs && (mknod(chantmp,mode, dev) == -1) )
			disaster(2,chantmp);

		if ( remake_devs && (chown(chantmp,0,gid) == -1))
			disaster(9,chantmp);

		if ( (muxfd=open(chantmp,O_RDWR)) == -1 ) {
			perror("mux open:");
			disaster(2,chantmp);
			(void) unlink(chantmp);
			return; 
		}


		if ( (devfd=open(drvtmp, O_RDWR)) == -1 ) {
			perror("driver open:");
			disaster(3,drvtmp);
			close(muxfd);
			(void) unlink(chantmp);
			return;
		}

		if ( ioctl(muxfd, I_PLINK, devfd) == -1 ) {
			perror("ioctl:");
			disaster(4,chantmp);
			close(muxfd);
			close(devfd);
			(void) unlink(chantmp);
			return;
		}

		if ( stat(vidtmp, &vidstatbuf) == -1 ) {
			perror("stat:");
			disaster(5,vidtmp);
			close(muxfd);
			close(devfd);

			continue;
		}

		if (! (vidstatbuf.st_mode&S_IFCHR)) {
			disaster(6,vidtmp);
			close(muxfd);
			close(devfd);

			continue;
		}

		if (devbuf != (dev_t *) NULL)
			*(devbuf + cmux_minor++) = vidstatbuf.st_rdev;
		
		close(muxfd);
		close(devfd);

		free(chantmp);
		free(vidtmp);
		free(drvtmp);
	}
}

disaster(typ,file)
int typ;
char *file;
{
	switch(typ) {
	case 1:
		fprintf(stderr,"WARNING: Cannot unlink %s\n",file);
		break;
	case 2:
		fprintf(stderr,"WARNING: Cannot make %s\n",file);
		break;
	case 3:
		fprintf(stderr,"WARNING: Cannot open %s\n",file);
		break;
	case 4:
		fprintf(stderr,"WARNING: Cannot I_LINK %s\n",file);
		break;
	case 5:
		fprintf(stderr,"WARNING: Cannot stat(2) %s\n",file);
		break;
	case 6:
		fprintf(stderr,"WARNING: file %s is not character special\n",file);
		break;
	case 7:
		fprintf(stderr,"WARNING: out of minor numbers for %s\n",file);
		break;
	case 8:
		fprintf(stderr,"WARNING: could not locate group information for group %s\n",file);
		break;
	case 9:
		fprintf(stderr,"WARNING: could not change ownership of %s\n",file);
		break;
	case 10:
		fprintf(stderr,"WARNING: ran out of space for video device buffer. Graphics will not work\n",file);
		break;
	case 11:
		fprintf(stderr,"WARNING: %s failed. Some special files may not have been removed\n",file);
		break;
	case 12:
		fprintf(stderr,"WARNING: file %s is truncated to zero length\n", file);
		break;
	default:
		fprintf(stderr,"WARNING: Unknown wsinit error type%d\n",typ);
		break;
	}
}
