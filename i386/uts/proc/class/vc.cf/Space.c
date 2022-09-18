/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/class/vc.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>	/* for tunable parameters */

#include <sys/types.h>
#include <sys/vcpriocntl.h>
#include <sys/vc.h>


#define VCGPUP0	0	/* Global priority for VC user priority 0 */
#define VCGPKP0	65	/* Global priority for VC kernel priority 0 */

short	vc_maxupri=VCMAXUPRI;


/*
 *  VP/ix processes do not sleep when idle, but go into a loop looking
 *  for keyboard input.  The ECT has heuristics which attempt to
 *  recognize this state and indicate to the kernel that the process
 *  is "busywaiting".  A busywaiting process should still be given
 *  time for two reasons.  Firstly the heuristic cannot be relied on
 *  to be accurate in all situations.  Secondly DOS programs often
 *  intercept the timer interrupt for various purposes such as
 *  maintaining on-screen clocks.  We recognize three different
 *  levels of busywaiting representing different levels of confidence
 *  that DOS is really idle.
 *
 *  Ideally a busywaiting process should be given just enough CPU
 *  time to do what it needs, once every interval at which it needs
 *  it.  Not knowing what these are, we choose to try to give it
 *  at least one time slice every second.  The assumption is that
 *  any timed event to be viewed by a user can be delayed by as much
 *  as one second without problems and that one clock interval is
 *  sufficient.
 *
 *  Busywaiting processes are normally run at priority 0, 1 or 2
 *  depending on their busywait level.  They all have the same
 *  global priority so that a VP/ix priority 1 or 2 process cannot
 *  take all available CPU time when a VP/ix priority 0 process is
 *  runnable.  The higher priority levels have longer timeslices
 *  because there is more chance that they are doing real work.
 *
 *  All the other priorities of the form x0, x1 or x2 are reserved
 *  for busywaiting processes which failed to get their timeslice.
 *  If a priority 0 process fails to receive its timeslice, it is
 *  boosted to priority 50.  From there it returns to 0 if it gets
 *  its timeslice, or 40 if it does not.  The expectation is that
 *  under normal situations the timeslice will be received.  If it
 *  is not, the system is too busy for on-screen clocks etc to be
 *  important.  Any time the process receives its timeslice it
 *  returns to priority 0, any time it does not its priority is
 *  further reduced until it reaches 10 where it stays until it
 *  receives its timeslice.  The same algorithm is true for priorities
 *  1 and 2 using levels x1 and x2.
 *
 *  At the elevated priority levels all busywaiting processes have
 *  timeslices of one clock tick per second.
 *
 *  To ensure the correct behavior of busywaiting processes it is
 *  important that no other process class uses global priority 0
 *  unless it also uses very short timeslices.
 *
 *  Note that the busywaiting algorithms tend to limit the number
 *  of VP/ix processes.  With a 100Hz clock, 100 busywaiting VP/ix
 *  processes can completely saturate a single CPU.  The useful
 *  limit for VP/ix processes is presumably rather lower.  On a
 *  processor capable of running that many processes 1/100 sec is
 *  much more CPU time than is necessary for a truly idle VP/ix process
 *  and the system clock can be set for a multiple of 100Hz with
 *  VP/ix processes capable of yielding the CPU on any tick, and
 *  normal Unix timer handling being done 100 times per second.
 *
 *  Non-busywaiting VP/ix processes generally use the remaining priority
 *  levels the same way that time-sharing processes do except that
 *  the top two levels are reserved for processes which have received
 *  pseudorupts and timeslices are defined as the desired number of
 *  ticks per second instead of per 5 seconds.
 *
 *  Processes with pending pseudorupts need to run in user mode long enough
 *  to turn the pseudorupts into virtual interrupts and emulate the
 *  effect of the interrupt, e.g. echoing keyboard characters.  Again,
 *  these priority levels should not be used by other process classes
 *  unless the requirements are similar.
 *
 *  Timeslices are expressed in ticks per second because the same real
 *  time events that affect busywaiting are equally important when a
 *  process is doing real processing.  As in the case of busywaiting,
 *  one-second accuracy is considered adequate.
 */
vcdpent_t	vc_dptbl[] = {
				  VCGPUP0,      1,    0,   33,   1,  50,
				  VCGPUP0,      2,    1,   33,   1,  51,
				  VCGPUP0,      3,    2,   33,   1,  52,

				  VCGPUP0+3,    20,   3,   13,   1,  13,
				  VCGPUP0+4,    20,   3,   14,   1,  14,
				  VCGPUP0+5,    20,   4,   15,   1,  15,
				  VCGPUP0+6,    20,   4,   16,   1,  16,
				  VCGPUP0+7,    20,   5,   17,   1,  17,
				  VCGPUP0+8,    20,   5,   18,   1,  18,
				  VCGPUP0+9,    20,   6,   19,   1,  19,

				  VCGPUP0+10,   1,    0,   33,   1,  10,
				  VCGPUP0+11,   1,    1,   33,   1,  11,
				  VCGPUP0+12,   1,    2,   33,   1,  12,

				  VCGPUP0+13,   16,   6,   23,   1,  23,
				  VCGPUP0+14,   16,   7,   24,   1,  24,
				  VCGPUP0+15,   16,   7,   25,   1,  25,
				  VCGPUP0+16,   16,   8,   26,   1,  26,
				  VCGPUP0+17,   16,   8,   27,   1,  27,
				  VCGPUP0+18,   16,   9,   28,   1,  28,
				  VCGPUP0+19,   16,   9,   29,   1,  29,

				  VCGPUP0+20,   1,    0,   33,   1,  10,
				  VCGPUP0+21,   1,    1,   33,   1,  11,
				  VCGPUP0+22,   1,    2,   33,   1,  12,

				  VCGPUP0+23,   12,  13,   33,   1,  33,
				  VCGPUP0+24,   12,  14,   34,   1,  34,
				  VCGPUP0+25,   12,  15,   35,   1,  35,
				  VCGPUP0+26,   12,  16,   36,   1,  36,
				  VCGPUP0+27,   12,  17,   37,   1,  37,
				  VCGPUP0+28,   12,  18,   38,   1,  38,
				  VCGPUP0+29,   12,  19,   39,   1,  39,

				  VCGPUP0+30,   1,   0,    33,   1,  20,
				  VCGPUP0+31,   1,   1,    33,   1,  21,
				  VCGPUP0+32,   1,   2,    33,   1,  22,

				  VCGPUP0+33,   8,   23,   43,   1,  43,
				  VCGPUP0+34,   8,   24,   44,   1,  44,
				  VCGPUP0+35,   8,   25,   45,   1,  45,
				  VCGPUP0+36,   8,   26,   46,   1,  46,
				  VCGPUP0+37,   8,   27,   47,   1,  47,
				  VCGPUP0+38,   8,   28,   48,   1,  48,
				  VCGPUP0+39,   8,   29,   49,   1,  49,

				  VCGPUP0+40,   1,   0,    33,   1,  30,
				  VCGPUP0+41,   1,   1,    33,   1,  31,
				  VCGPUP0+42,   1,   2,    33,   1,  32,

				  VCGPUP0+43,   4,   33,   53,   1,  53,
				  VCGPUP0+44,   4,   34,   53,   1,  53,
				  VCGPUP0+45,   4,   35,   54,   1,  54,
				  VCGPUP0+46,   4,   36,   54,   1,  54,
				  VCGPUP0+47,   4,   37,   55,   1,  55,
				  VCGPUP0+48,   4,   38,   55,   1,  55,
				  VCGPUP0+49,   4,   39,   56,   1,  56,

				  VCGPUP0+50,   1,   0,    33,   1,  40,
				  VCGPUP0+51,   1,   1,    33,   1,  41,
				  VCGPUP0+52,   1,   2,    33,   1,  42,

				  VCGPUP0+53,   2,   43,   56,   1,  56,
				  VCGPUP0+54,   2,   44,   56,   1,  56,
				  VCGPUP0+55,   2,   45,   57,   1,  57,
				  VCGPUP0+56,   2,   46,   57,   1,  57,
				  VCGPUP0+57,   2,   47,   57,   1,  57,
/*
 *  The next two priorities are for processes which have recently
 *  received pseudorupts.  The two levels allow for a distinction
 *  between urgent and non-urgent pseudorupts.  The intent is to
 *  let a process receiving a pseudorupt run in user mode long
 *  enough to handle it.
 */
				  VCGPUP0+63,   1,   57,   57,   1,  58,
				  VCGPUP0+64,   1,   57,   57,   1,  59
				  };

int	vc_kmdpris[]
		      = {
			VCGPKP0+0,  VCGPKP0+1,  VCGPKP0+2,  VCGPKP0+3,
			VCGPKP0+4,  VCGPKP0+5,  VCGPKP0+6,  VCGPKP0+7,
			VCGPKP0+8,  VCGPKP0+9,  VCGPKP0+10, VCGPKP0+11,
			VCGPKP0+12, VCGPKP0+13, VCGPKP0+14, VCGPKP0+15,
			VCGPKP0+16, VCGPKP0+17, VCGPKP0+18, VCGPKP0+19,
			VCGPKP0+20, VCGPKP0+21, VCGPKP0+22, VCGPKP0+23,
			VCGPKP0+24, VCGPKP0+25, VCGPKP0+26, VCGPKP0+27,
			VCGPKP0+28, VCGPKP0+29, VCGPKP0+30, VCGPKP0+31,
			VCGPKP0+32, VCGPKP0+33, VCGPKP0+34, VCGPKP0+35,
			VCGPKP0+36, VCGPKP0+37, VCGPKP0+38, VCGPKP0+39
			};

short	vc_maxkmdpri = sizeof(vc_kmdpris)/sizeof(int) - 1;
short	vc_maxumdpri = sizeof(vc_dptbl)/sizeof(vcdpent_t) - 1;
