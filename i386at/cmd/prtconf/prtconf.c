/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtconf:prtconf.c	1.7.5.4"
#ident "$Header: prtconf.c 1.1 91/08/28 $"

#include <sys/errno.h>
#include <sys/cram.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/vtoc.h>
#include <sys/sdi_edt.h>
#include <limits.h>

extern int errno;

main()
{
        unsigned char x;
        struct scsi_edt	sedt;
        int fd, drivea, driveb, memsize;
	int	fd_edt, i;
	int disk_cnt = 0;
	int found_disk = 0;
	char	*sp;
        long size;
        unsigned char buf[2];
	unsigned long diskparms();
	if ((fd = open("/dev/cram",O_RDONLY)) == -1 ) {
                printf("can't open cram, errno %d\n",errno);
                exit(-1);
        }
        printf ("\n    SYSTEM CONFIGURATION:\n\n");

        buf[0] = EMHIGH;
        crio(fd,buf);
        memsize = buf[1] / 4 + 1;
        printf("    Memory Size: %d Megabytes\n", memsize);
        printf ("    System Peripherals:\n\n");

        /* check for floppy drive */
        buf[0] = DDTB;
        crio(fd,buf);
        drivea = ((buf[1] >> 4) & 0x0f);
        (void) printflp(0, drivea);
        driveb = (buf[1] & 0x0f);
        (void) printflp(1, driveb);

/* check for devices in edt - no error msg if we fail */
	if ((fd_edt = open("/etc/scsi/pdi_edt", O_RDONLY)) != -1) {
		int tid = 0;
		int cid = 0;
		while (read(fd_edt, &sedt, sizeof(struct scsi_edt)) != NULL) {
			if (sedt.tc_equip && strcmp(sedt.drv_name, "VOID")) {
				char typename[30];
				if (!strcmp(sedt.drv_name, "SW01"))
					strcpy(typename, "WORM Device");
				else if (!strcmp(sedt.drv_name, "SC01"))
					strcpy(typename, "CD-ROM Device - ");
				else if (!strcmp(sedt.drv_name, "ST01"))
					strcpy(typename, "Tape Device - ");
				else if (!strcmp(sedt.drv_name, "SD01")) {
					found_disk++;
					strcpy(typename, "Hard Disk");
				 } else
					strcpy(typename, "Unknown Device Type - ");

				if (strcmp(sedt.drv_name, "SD01"))
					(void) printf("\t%s%s\n", typename, sedt.tc_inquiry);
				else {
					int size;
					for (i=0;i < (int)sedt.n_lus;i++) {
						size  = diskparms(cid, tid, i);
						if (size != (unsigned long)-1)
							(void) printf("\t%s%d - %ld Megabyte Disk\n", typename, disk_cnt++, size);
					}

				}

			}
			if ( ++tid > 7) {
				/* SCSI disk nodes are logically
				** assigned so the controller number
				** is only incremented when a disk
				** is found in the current HA.
				*/
				if(found_disk)
					cid++;
				found_disk= 0;
				tid = 0;
			}
		}

		(void) close(fd_edt);
	}

	/* check 80387 chip */
        buf[0] = EB;
        crio(fd,buf);
        if (buf[1] & 0x02)
                printf("	80387 Math Processor\n");

	/* last but not least add a blank line for asthetic appeal */


	printf("\n\n");
	exit(0);
}

/* get size of the disk */
unsigned long 
diskparms(cid, tid, did)
int cid; /* controller id */
int tid; /* target id */
int did; /* disk id */
{
int hd;
struct disk_parms dp;
char pname[PATH_MAX];
unsigned long size;

	sprintf(pname, "/dev/rdsk/c%dt%dd%ds0", cid, tid, did);
	if (((hd = open(pname, 2)) != -1) && 
	   (ioctl(hd, V_GETPARMS, &dp) != -1)) 
		size = ((long)(dp.dp_cyls*dp.dp_sectors*512*dp.dp_heads))/(long)(1024*1024);
	else
		size = (unsigned long)-1;
	close(hd);
	return (size);
}

printflp(drivenum, drivetype)
int drivenum;
int drivetype;
{
        switch (drivetype) {
                case 1:
                        printf("        Floppy Disk%d - 360KB 5.25\n", drivenum);
                        break;
                case 2:
                        printf("        Floppy Disk%d - 1.2MB 5.25\n", drivenum);
                        break;
                case 3:
                        printf("        Floppy Disk%d - 720KB 5.25\n", drivenum);
                        break;
                case 4:
                        printf("        Floppy Disk%d - 1.44MB 3.5\n", drivenum);
                        break;
                default:
                        break;
        }
        return;
}

crio(fd,buf)
int fd;
char *buf;
{
        if(ioctl(fd,CMOSREAD, buf) < 0)
        {
                printf("can't open iocntl, cmd=%x, errno %d\n",buf[0], errno);
                exit(-1);
        }
}
