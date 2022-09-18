/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)setclk:i386/cmd/setclk/setclk.c	1.4.10.3"

/***************************************************************************
 * Command : setclk
 * Inheritable Privileges : P_SYSOPS
 *       Fixed Privileges : None
 * Notes:
 *
 ***************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <priv.h>
#include <sys/sysi86.h>
#include <sys/uadmin.h>
#include <sys/rtc.h>
#include <sys/errno.h>
#include <time.h>

#define	dysize(A) (((A)%4)? 365: 366)	/* number of days per year */

static int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int month, day, year, hour, mminute, second, dayweek;
static time_t timbuf, oldtime;
static int otzone;

static int rtc_ng;

main()
{
	struct rtc_t clk;
	extern time_t tointernal();
	struct tm *tmp;
	extern errno;

	if (sysi86(RTODC, &clk, 0) < 0) {
		if (errno == EINVAL) {
			printf("RTODC not implemented in system\n");
			printf("unable to set time from the Real Time Clock\n");
			exit(0);
		}
		rtc_ng = 1;
	}
	time(&oldtime);
	tmp = localtime(&oldtime);
	otzone = timezone;

	if (rtc_ng) {	/* time-of-day clock is no good */
		printf("\n		Time of Day Clock needs Restoring:\n");
		printf("		Change using \"date\" utility\n");
		exit(0);
	}
	timbuf = tointernal(&clk);
	sysi86(STIME, timbuf);
}

#define unhexize(A)	(((((A)>>4)&0xF)*10)+((A)&0xF))

time_t
tointernal(clkp)
 register struct rtc_t *clkp;
{
 time_t thetime, correction;
 register int i, moredays;
 struct tm *tmp;

	second = unhexize(clkp->rtc_sec);
	mminute = unhexize(clkp->rtc_min);
	hour = unhexize(clkp->rtc_hr);
	day = unhexize(clkp->rtc_dom);
	dayweek = unhexize(clkp->rtc_dow);
	month = unhexize(clkp->rtc_mon);
	year = unhexize(clkp->rtc_yr);
	if (year < 70) year += 100;
	thetime = second + 60 * mminute + 3600 * hour;
	thetime += (day-1) * 86400L;
	if (dysize(year) == 366)
		dmsize[1] = 29;
	moredays = 0;
	for (i = month-1; --i >= 0; )
		moredays += dmsize[i];
	dmsize[1] = 28;
	for (i = 70; i < year; i++)
		moredays += dysize(i);
	thetime += moredays * 86400L;
        thetime += otzone;
        tmp = localtime(&thetime);
	correction = timezone;
        if (tmp->tm_isdst) {
	  thetime += altzone - timezone;
	  correction = altzone;
	}
	uadmin(A_CLOCK, correction, 0);
	return(thetime);
}
