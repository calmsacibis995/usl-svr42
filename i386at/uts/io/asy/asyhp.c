/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */

#ident	"@(#)uts-x86at:io/asy/asyhp.c	1.14"
#ident	"$Header: $"

#ifndef lint
static char asyhp_copyright[] = "Copyright Intel Corporation xxxxxx";
#endif /* lint */

/*
 * This driver forms the hardware dependent part of the serial driver.
 * It uses the iasy driver interface to comminicate to the modules upstream.
 *
 * It supports National 16550 and compatible chips. The chip has 16-deep
 * fifo. The driver writes as many as 16  characters at a time when
 * transmitter empty interrupt is received. Also, the chip is programmed
 * to interrupt when it the fifo level reaches 14 or it timesout.
 */

#include "util/types.h"
#include "svc/errno.h"
#include "util/cmn_err.h"
#include "proc/signal.h"
#include "util/param.h"
#include "io/conf.h"
#include "io/stream.h"
#include "io/termio.h"
#include "io/strtty.h"
#include "io/asy/iasy.h"
#include "io/asy/asyhp.h"
#include "io/asy/asy.h"
#include "io/ddi.h"
#include "io/tty.h"
#ifdef VPIX
#include "proc/proc.h"
#include "proc/tss.h"
#include "vpix/v86.h"
#include "util/inline.h"
#endif

extern	int	p_asyhp0;
struct strtty *asyhp_tty;
extern int asyhp_cnt;			/* How many ports are configured */
int	asyhpproc();
int	asyhpgetchar();
void	asyhpputchar(); 
extern	int	asyhpgetchar2();
extern	void	asyhpputchar2();
void asyhpwakeup();
extern	int	iasy_console;

#ifdef VPIX
int	asyhpvmproc();
#endif /* VPIX */

#ifdef VPIX
#define MSRWORD FRERROR
#define LSRWORD PERROR
extern v86_t *iasystash[];	/* save proc.p_v86 here for psuedorupts */
extern int iasyintmask[];	/* which pseudorupt to give */
extern struct termss iasyss[];	/* start/stop characters */
extern int   iasy_closing[];
extern char iasy_opened[];
extern int iasy_v86_prp_pd[];
extern  int     validproc();
#endif /* VPIX */

#ifdef MERGE386
extern  int     asy_is_assigned();
extern  int     com_ppiioctl();
extern  int     merge386enable;
#endif /* MERGE386 */

/* Routine for putting a char in the input c-list */
#define PutInChar(tp, c) if ((tp)->t_in.bu_ptr != NULL) { \
		*(tp->t_in.bu_ptr++) = c; \
		if (--tp->t_in.bu_cnt == 0) \
			iasy_input(tp, L_BUF); \
	}

unsigned int asyhp_alive=0;
unsigned int asyhpinitialzed=0;
int 	asyhpinitialized = 0;
extern  struct  asyhp asyhptab[];   /* asyhp structs for each port */
extern unsigned int asyhp_num;
extern unsigned int asyhp_sminor;
extern struct strtty asy_tty[];		/* iasy_tty changed to asy_tty for merge */
int	asyhp_id=0;

extern int	asyhp_outchars[];

#define OUTFIFOLIMIT 16 /*	Number of characters that we will ship whenever
							the transmitter is empty. */

/*
 * Baud rate table. Indexed by #defines found in io/termios.h
 */

#define MAXBAUDS 17
ushort asyhpspdtab[] = {
	0,	/* 0 baud rate */
	0x900,	/* 50 baud rate */
	0x600,	/* 75 baud rate */
	0x417,	/* 110 baud rate ( %0.026 ) */
	0x359,	/* 134.5 baud rate ( %0.058 ) */
	0x300,	/* 150 baud rate */
	0x240,	/* 200 baud rate */
	0x180,	/* 300 baud rate */
	0x0c0,	/* 600 baud rate */
	0x060,	/* 1200 baud rate */
	0x040,	/* 1800 baud rate */
	0x030,	/* 2400 baud rate */
	0x018,	/* 4800 baud rate */
	0x00c,	/* 9600 baud rate */
	0x006,	/* 19200 baud rate */
	0x003	/* 38400 baud rate */
};

/*
 * asyhpinit() -- initialize driver with interrupts off
 *
 *  This is called before the interrupts are turned on to initalize
 *  the device.  Reset the puppy and program it do asyhpnc IO and to
 *  give interrupts for input and output.
 */
void
asyhpinit()
{	
	int	i;
	int	unit;
	struct asyhp *asyhp;
	int	(*vmproc)() = 0;	/* VPIX support func. ptr */

	if (asyhpinitialized)
		return;
	asyhpinitialized=1;

	/* Are we using one of the faster USARTs */
	for (unit=0; unit<asyhp_num; unit++) {
		asyhp = &asyhptab[unit];
		/* 
		 * Bit 4 & 5 of ISR are wired low. If bit 4 or 5 appears on inb(),
		 * board is not there.
		 */ 
        	if ((inb(asyhp->asyhp_isr) & 0x30)) {
            		asyhp->asyhp_flags &= ~ASYHERE;
                        continue; /* no serial adapter here */
        	}
        	asyhp->asyhp_flags |= ASYHERE;

		/*
                 * Reset all registors for soft restart.
                 */

                outb(asyhp->asyhp_icr,0x00);
                outb(asyhp->asyhp_isr,0x00);
                outb(asyhp->asyhp_lcr,0x00);
                outb(asyhp->asyhp_mcr,0x00);


		outb(asyhp->asyhp_isr,0xc1);
		if ((inb(asyhp->asyhp_isr) & 0xc0) == 0xc0) {
			cmn_err (CE_CONT, "asyhpinit: Found National 16550\n");
			++asyhp_alive;
			asyhp->asyhp_flags |= ASY16550; /* 16550 chip present */
   		 	outb(asyhp->asyhp_isr, 0x0);    /* clear both fifos */
			outb(asyhp->asyhp_isr,FIFOEN);  /* fifo trigger set */

			/*
                 	* Reset all interrupts for soft restart.
                 	*/

                	(void)inb(asyhp->asyhp_isr);
                	(void)inb(asyhp->asyhp_lsr);
                	(void)inb(asyhp->asyhp_msr);
                	(void)inb(asyhp->asyhp_dat);

		}
		else
			cmn_err (CE_NOTE, "asyhp initialisation failed : National 16550 NOT found");
	}
	
	if (!asyhp_alive)
		return;
#ifdef VPIX
	vmproc = asyhpvmproc;
#else
	vmproc = (int (*)()NULL;
#endif /* VPIX */

	/* Register terminal server. */
	if ((asyhp_sminor == 0) && (iasy_console == 0)){
		asyhp_id = iasy_register(asyhp_sminor, asyhp_num, asyhpproc, iasyhwdep, asyhpgetchar, asyhpputchar, vmproc);
	}else{
		if ((asyhp_sminor <= iasy_console) && (iasy_console <= asyhp_num) && (iasy_console == 1)){
			asyhp_id = iasy_register(asyhp_sminor, asyhp_num, asyhpproc, iasyhwdep, asyhpgetchar2, asyhpputchar2, vmproc);
		}else{
			asyhp_id = iasy_register(asyhp_sminor, asyhp_num, asyhpproc, iasyhwdep, 0, 0, vmproc);
		}
	}

	asyhp_tty = IASY_UNIT_TO_TP(asyhp_id, 0);

}


/*
 *	INTERRUPT TIME EXECUTION THREAD
*/
/*
 * asyhpintr() -- process device interrupt
 *
 * This procedure is called by system with interrupts off
 * when the USART interrupts the processor. 
 *
 */
#define	ICNT	50
/*ARGSUSED*/
void
asyhpintr(vect)
int	vect;			/* Unused */
{	
	register struct	strtty *tp;	/* strtty structure */
	register	unit;
	struct asyhp *asyhp;
	ushort	c;
	unsigned char   interrupt_id, line_status, modem_status;
	ushort chars[ICNT];
	ushort charcount;

    for (asyhp = &asyhptab[0], unit =0;  unit < asyhp_num ; asyhp++, unit++)
        if( (asyhp->asyhp_vect == vect) && ( asyhp->asyhp_flags & ASYHERE))
            break;

    if (unit >= asyhp_num)
        return;

#ifdef MERGE386
	/* needed for com port attachment */
        if(merge386enable)
                if (asyhp->asyhp_ppi_func && (*asyhp->asyhp_ppi_func) (asyhp, -1))
                        return;

#endif /* MERGE386 */

	for (;;) {
nextloop:
		interrupt_id =   inb(asyhp->asyhp_isr) & 0x0f; 
		line_status  =   inb(asyhp->asyhp_lsr);
		modem_status =   inb(asyhp->asyhp_msr);
	
		tp = IASY_UNIT_TO_TP(asyhp_id, unit);
		if ((interrupt_id == ISR_RxRDY) || (interrupt_id == ISR_FFTMOUT) || 
					(line_status == LSR_RCA) || (interrupt_id == ISR_RSTATUS)) {
	
			charcount=0;
			while (line_status & LSR_RCA) {
   	         	drv_setparm(SYSRINT, 1);
				c = inb(asyhp->asyhp_dat) & 0xff;
				
				if ( (tp->t_cflag & CREAD) != CREAD )  {
					if (asyhp->asyhp_flags & ASY16550) {
						/* Clear the receiver fifo */
						while ((line_status=inb(asyhp->asyhp_lsr)) & LSR_RCA){
							inb(asyhp->asyhp_dat);
						}
					}
					continue;
				}

#ifdef MERGE386
			/* Needed for direct attachment of the serial port */
			if(merge386enable) {
				if (asyhp->asyhp_ppi_func && (*asyhp->asyhp_ppi_func) (asyhp, c)) {
					line_status = inb(asyhp->asyhp_lsr);
					continue;
				}
			}
#endif /* MERGE386 */

				if (line_status&(LSR_PARERR|LSR_FRMERR|LSR_BRKDET|LSR_OVRRUN)) {
#ifdef VPIX
					if ((tp->t_iflag & (DOSMODE | PARMRK)) == 
											(DOSMODE | PARMRK)){
						c = (line_status | (unit<<8) | LSRWORD);
						line_status=0;
					}
#endif /* VPIX */
					if (line_status & LSR_PARERR)
						c |= PERROR;
					else if (line_status & LSR_FRMERR|LSR_BRKDET)
						c |= FRERROR;
					else if (line_status & LSR_OVRRUN)
						c |= OVERRUN;
				}

				chars[charcount++]=c;
				line_status  =   inb(asyhp->asyhp_lsr);
			}

			if (charcount)
				asyhpProcessInChar(tp, chars, charcount);

		} else
		if (interrupt_id == ISR_TxRDY || (line_status & LSR_XHRE) 
									&& (tp->t_state & BUSY)){
			drv_setparm(SYSXINT, 1);
			asyhp_outchars[unit]=0;
			tp->t_state &= ~BUSY;       /* clear busy bit */
			(void) asyhpproc(tp, T_OUTPUT);
		} else
		if (interrupt_id == ISR_MSTATUS)  
			asyhpmodem(tp);
		else
			return;
	}
}

/*
 * asyhpProcessInChar() -- do the input processing for incoming characters.
 *
 * This is called from the interrupt routine to handle all the per
 * character processing stuff.
*/
asyhpProcessInChar(tp, chars, cnt)
register struct strtty *tp;
ushort	chars[];
ushort	cnt;
{
	int	dev;
	ushort	c;
	ushort	charcount;

	if (!(tp->t_state & ISOPEN))
			return;

	dev = tp->t_dev / 2;

#ifdef VPIX
	if ( (tp->t_iflag & DOSMODE) && iasystash[dev]) {
		if( validproc(iasy_v86_prp_pd[2*iasychan(dev)], iasy_v86_prp_pd[(2*iasychan(dev)) + 1]))
			v86setint(iasystash[dev], iasyintmask[dev]);
		else {
			tp->t_iflag &= ~DOSMODE;
			iasystash[iasychan(dev)] = 0;
			iasyintmask[iasychan(dev)] = 0;
			iasy_v86_prp_pd[2*iasychan(dev)] = 0;
			iasy_v86_prp_pd[(2*iasychan(dev)) + 1] = 0;
		}
	}
#endif /* VPIX */
	
	for (charcount=0; charcount < cnt; charcount++) {

		if (tp->t_in.bu_cnt < 3) {
			/*
			 * Not enough room in the buffer, try to ship 
			 * this buffer upstream.
			 */
			if (iasy_input(tp, L_BUF)) {
				break; /* Drop characters */
			}
		}
	
		c=chars[charcount];
	
		if (c & PERROR && !(tp->t_iflag & INPCK)) {
			if( (tp->t_cflag & (PARODD|PARENB)) != (PARODD|PARENB)
				&& (( c & 0377 ) != 0 ))
					c &= ~PERROR;
		}
	
		if (c & (FRERROR|PERROR|OVERRUN)) {
	
#ifdef VPIX
			if ((tp->t_iflag & (DOSMODE | PARMRK)) ==
								(DOSMODE | PARMRK)) {
				PutInChar(tp, 0377);
				if (c & MSRWORD)
					PutInChar(tp, 2);
				if (c & LSRWORD)
					PutInChar(tp, 1);
				PutInChar(tp, (char)(c));
			}
#endif /* VPIX */
	
			if ((c & 0377) == 0) {
				if (!(tp->t_iflag & IGNBRK)) {
					iasy_input(tp, L_BUF); /* Ship upstream whatever we
											  have seen so far.	*/
					iasy_input(tp, L_BREAK);
	                continue;
	            } else
					continue;
			} else if (tp->t_iflag & IGNPAR) {
				continue;
			}
	
	        if (tp->t_iflag & PARMRK) {
	        	/* Mark the Parity errors */
				PutInChar(tp, 0377);
				PutInChar(tp, 0);
				PutInChar(tp, c&0377);
				continue;
			} else {
	            /* Send up a NULL character */
				PutInChar(tp, 0);
				continue;
			}
		}
		else {	
			if (tp->t_iflag & ISTRIP) {
				c &= 0177;
			} else {
				if (c == 0377 && tp->t_iflag&PARMRK) {
					/* if marking parity errors, this character gets doubled */
					PutInChar(tp, 0377);
					PutInChar(tp, 0377);
					continue;
				}
			}
		}
	
		if (tp->t_iflag & IXON) {
			/* if stopped, see if to turn on output */
#ifdef VPIX
			if (tp->t_state & TTSTOP) {
				if (c == iasyss[dev].ss_start ||
							tp->t_iflag&IXANY) {
					(void) asyhpproc(tp, T_RESUME);
				}
			} else {
				/* maybe we're supposed to stop output */
				if (c == iasyss[dev].ss_stop && (c != 0)) {
					(void) asyhpproc(tp, T_SUSPEND);
				}
			}
			if ((c == iasyss[dev].ss_stop) || (c == iasyss[dev].ss_start))
				continue;
#else
			if (tp->t_state & TTSTOP) {
				if (c == tp->t_cc[VSTART] ||
							ap->asyhp_iflag&IXANY) {
					(void) asyhpproc(tp, T_RESUME);
				}
			} else {
				/* maybe we're supposed to stop output */
				if (c == tp->t_cc[VSTOP] && (c != 0)) {
					(void) asyhpproc(tp, T_SUSPEND);
				}
			}
			if ((c == tp->t_cc[VSTOP]) || (c == tp->t_cc[VSTART]))
				continue;
#endif
		}
	
		PutInChar(tp, c);
	}

	/* Ship any characters upstream */
	iasy_input(tp, L_BUF);
}

#ifdef VPIX
int 
asyhpvmproc(tp, cmd, argp1, argp2)
struct strtty *tp;
int	cmd;
char *argp1;
int *argp2;
{
	int	s,rq;
	struct asyhp *asyhp;
	char	val, val2;
	int	efl;
	int	ret_val=0;

	s = splstr();
    /*
     * get device number and control port
     */
    asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id,tp)];
	switch (cmd) {
#ifdef MERGE386
		case COMPPIIOCTL: /* Do com_ppiioctl() */
			ret_val = com_ppiioctl( OTHERQ(tp->t_rdqp), (mblk_t *)argp1, 
										&asyhptab[tp->t_dev/2], (*argp2));
			break;
#endif
		case AIOCDOSMODE:
			val = inb(asyhp->asyhp_icr ) | ICR_MIEN;
			outb(asyhp->asyhp_icr , val);
			break;
	    case AIOCNONDOSMODE:
	    case AIOCINFO:
	    case AIOCSETSS:
	    case AIOCINTTYPE:
			break;
	    case AIOCSERIALOUT:
			rq = fubyte(argp1);
			if (rq & SIO_MASK(SO_DIVLLSB)) {
				val = fubyte(argp1+SO_DIVLLSB);
				efl = intr_disable();
				val2 = inb(asyhp->asyhp_lcr);
				outb(asyhp->asyhp_lcr, val2 | LCR_DLAB);
				outb(asyhp->asyhp_dat, val);
				outb(asyhp->asyhp_lcr, val2 & ~LCR_DLAB);
				intr_restore(efl);
			}
			if (rq & SIO_MASK(SO_DIVLMSB)) {
				val = fubyte(argp1+SO_DIVLMSB);
				efl = intr_disable();
				val2 = inb(asyhp->asyhp_lcr);
				outb(asyhp->asyhp_lcr, val2 | LCR_DLAB);
				outb(asyhp->asyhp_icr, val);
				outb(asyhp->asyhp_lcr, val2 & ~LCR_DLAB);
				intr_restore(efl);
			}
			if (rq & SIO_MASK(SO_LCR)) {
				val = fubyte(argp1+SO_LCR);
				outb(asyhp->asyhp_lcr, val);
			}
			if (rq & SIO_MASK(SO_MCR)) {
				val = fubyte(argp1+SO_MCR);
				/* force OUT2 on to preserve interrupts */
				outb(asyhp->asyhp_mcr, val | MCR_OUT2);
			}
			break;
	    case AIOCSERIALIN:
			rq = fubyte(argp1);
			if (rq & SIO_MASK(SI_MSR)) {
				subyte(argp1+SI_MSR,inb(asyhp->asyhp_mcr));
			}
			break;
		default:
			cmn_err(CE_NOTE, "asyhpvmproc: Unknown command");
			break;
	}
	splx(s);
	return(ret_val);
}
#endif /* VPIX */

/*
 * asyhpproc() -- low level device dependant operations
 *
 * It is called at both task time by the line discipline routines,
 * and at interrupt time by asyhpintr().
 * asyhpproc handles any device dependent functions required
 * upon suspending, resuming, blocking, or unblocking output; flushing
 * the input or output queues; timing out; sending break characters,
 * or starting output.
 */
int
asyhpproc(tp, cmd)
register struct strtty *tp;
int	cmd;
{
	int s;
	unsigned char line_ctl;
	int	dev;
	struct asyhp *asyhp;
	int	ret_val=0;
	unsigned char fcr_val;
	char	val, val2;
	unsigned char line_status;

	s = splstr();
    /*
     * get device number and control port
     */
    asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id,tp)];
	dev = (tp->t_dev / 2);

	switch(cmd)  {
	case T_WFLUSH:			/* flush output queue */
		fcr_val=inb(asyhp->asyhp_isr);
		outb(asyhp->asyhp_isr, (fcr_val|0x04));	/* Flush XMIT FIFO */
		tp->t_out.bu_cnt = 0;   /* abandon this buffer */
	case T_RESUME:			/* resume output */
 		if ( (asyhp->asyhp_flags & (HWDEV | HWFLWO) == (HWDEV | HWFLWO) ) ){
			break;
		}
		tp->t_state &= ~TTSTOP;
		asyhpstartio(tp);
		break;
	case T_SUSPEND:			/* suspend output */
		tp->t_state |= TTSTOP;
		break;
	case T_BLOCK:			/* send stop char */
		tp->t_state &= ~TTXON;
		tp->t_state |= TBLOCK|TTXOFF;
		asyhpstartio(tp);
		break;
	case T_RFLUSH:			/* flush input queue */
		fcr_val=inb(asyhp->asyhp_isr);
		outb(asyhp->asyhp_isr, (fcr_val|0x02));	/* Flush RCVR FIFO */
		tp->t_in.bu_cnt = IASY_BUFSZ;
                tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		if (!(tp->t_state & TBLOCK))
			break;
		/* FALLTHROUGH */
	case T_UNBLOCK:			/* send start char */
		tp->t_state &= ~(TTXOFF | TBLOCK);
		tp->t_state |= TTXON;
		asyhpstartio(tp);
		break;
	case T_TIME:			/* time out */
		tp->t_state &= ~TIMEOUT;
        line_ctl = inb(asyhp->asyhp_lcr);
        outb( asyhp->asyhp_lcr, line_ctl & ~LCR_SETBREAK );
		asyhpstartio(tp);
		break;
	case T_BREAK:			/* send null for .25 sec */
		line_status=inb(asyhp->asyhp_lsr);
                while (!(line_status & LSR_XSRE)) {
                        drv_usecwait(10);
                        line_status=inb(asyhp->asyhp_lsr);
                }
		tp->t_state |= TIMEOUT;
        line_ctl = inb(asyhp->asyhp_lcr);
        outb(asyhp->asyhp_lcr, line_ctl | LCR_SETBREAK);
		(void) timeout(asyhpwakeup, (caddr_t)tp, HZ/4);
		break;
	case T_OUTPUT:			/* start output */
		asyhpstartio(tp);
		break;
	case T_CONNECT:			/* connect to the server */

#ifdef MERGE386
        if (asyhp->asyhp_ppi_func && asy_is_assigned(asyhp) ) {
                ret_val = EACCES;
				break;
        }
#endif /* MERGE386 */

		if (!(asyhp->asyhp_flags & ASYHERE)) {
			ret_val=ENXIO;
			break;
		}

		if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {
			
			outb(asyhp->asyhp_lcr, LCR_DLAB);
			outb(asyhp->asyhp_dat, asyhpspdtab[B1200] & 0xff);
			outb(asyhp->asyhp_icr, asyhpspdtab[B1200]  >> 8);
			outb(asyhp->asyhp_lcr, LCR_BITS8); 
			outb(asyhp->asyhp_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	

			asyhp_reset(tp);
			outb(asyhp->asyhp_isr,FIFOEN);  /* fifo trigger set */
			asyhpparam(tp);
			if (tp->t_dev % 2) {
				asyhp->asyhp_flags |= HWDEV;
				asyhp->asyhp_flags &= ~(HWFLWO);
			}
			else {
				asyhp->asyhp_flags &= ~(HWDEV|HWFLWO);
			}
		}
/*
		else {
			if ((asyhp->asyhp_flags & HWDEV) == HWDEV) {
				if( (tp->t_dev % 2 ) == 0) {
					ret_val = EBUSY;
					splx(s);
					break;
				}
			}
			else
				if (tp->t_dev % 2) {
					ret_val = EBUSY;
					splx(s);
					break;
				}
		}
*/

		asyhpSetDTR(asyhp);

		if(tp->t_cflag & CLOCAL)
			tp->t_state |= CARR_ON;
		else {
			if (inb(asyhp->asyhp_msr) & MSR_DCD) {
				tp->t_state |= CARR_ON;
				iasy_carrier(tp);
			} else
				tp->t_state &= ~CARR_ON;
		}     

		break;

	case T_PARM:			/* output parameters */
		if (!(inb(asyhp->asyhp_lsr) & LSR_XSRE))
			/* Wait for one character time for Transmitter
			   shift register to get empty. */
			ret_val = iasy_ctime(tp, 1);
		else
			ret_val = asyhpparam(tp);
		break;
	case T_DISCONNECT:
/*
		line_status=inb(asyhp->asyhp_lsr);
		while (!(line_status & LSR_XSRE)) {
			drv_usecwait(10);
			line_status=inb(asyhp->asyhp_lsr);
		}
*/
		outb (asyhp->asyhp_mcr, MCR_OUT2);
		break;
	case T_TRLVL1:
                outb (asyhp->asyhp_isr, 0x0);
                outb (asyhp->asyhp_isr, TRLVL1);
                break;
        case T_TRLVL2:
                outb (asyhp->asyhp_isr, 0x0);
                outb (asyhp->asyhp_isr, TRLVL2);
                break;
        case T_TRLVL3:
                outb (asyhp->asyhp_isr, 0x0);
                outb (asyhp->asyhp_isr, TRLVL3);
                break;
        case T_TRLVL4:
                outb (asyhp->asyhp_isr, 0x0);
                outb (asyhp->asyhp_isr, TRLVL4);
                break;
	case T_SWTCH:
		break;
	default:
		break;
	};	/* end switch */
	splx(s);
	return(ret_val);
}

/*
 * asyhpstartio() -- start output on an serial channel if needed.
 *
 * Get a character from the character queue, output it to the
 * channel and set the BUSY flag. The BUSY flag gets reset by asyhpintr
 * when the character has been transmitted.
 */
asyhpstartio(tp)
register struct strtty *tp;
{
	register struct t_buf *tbuf;
	struct	asyhp	*asyhp;
	int	unit;
	int	*charcount;

	unit = IASY_TP_TO_UNIT(asyhp_id, tp);
	asyhp = &asyhptab[unit];
	charcount = &asyhp_outchars[unit];
	
	/* busy or timing out? or stopped?? */
	if(tp->t_state&(TIMEOUT|BUSY|TTSTOP))	{
		return;			/* wait until a more opportune time */
	}

	if (tp->t_state & (TTXON|TTXOFF)) {
		tp->t_state |= BUSY;
		if (tp->t_state & TTXON) {
			tp->t_state &= ~TTXON;
	 		if ( asyhp->asyhp_flags & HWDEV ) {
				char mcr = inb(asyhp->asyhp_mcr);
				outb(asyhp->asyhp_mcr, mcr | MCR_RTS);
 			} 
#ifdef VPIX
		 	outb(asyhp->asyhp_dat, CSTART);
#else
 			outb(asyhp->asyhp_dat, tp->t_cc[VSTART]);
#endif /* VPIX */
			++(*charcount);;
		} else {
			tp->t_state &= ~TTXOFF;
#ifdef VPIX
	 		outb(asyhp->asyhp_dat, CSTOP);
#else
			outb(asyhp->asyhp_dat, tp->t_cc[VSTOP]);
#endif /* VPIX */
			++(*charcount);
 			if ( asyhp->asyhp_flags & HWDEV ) {
				char mcr = inb(asyhp->asyhp_mcr);
				outb(asyhp->asyhp_mcr, mcr & ~MCR_RTS);
 			} 
		}
		return;
	}

	tbuf = &tp->t_out;
	while ((*charcount) < (OUTFIFOLIMIT)) {
		if (!(tbuf->bu_ptr && tbuf->bu_cnt)) {
			if (!(CPRES & iasy_output(tp)))
				return;
		}

		tp->t_state |= BUSY;
		outb(asyhp->asyhp_dat, (char)(*(tbuf->bu_ptr++)));
		++(*charcount);
		--(tbuf->bu_cnt);
	}
}


/*
/* Param detects modifications to the line characteristics and reprograms 
 */
asyhpparam(tp)
struct strtty *tp;
{
	struct asyhp *asyhp;
	unsigned int flags;  /* To ease access to c_flags    */
	int s;      /* Index to conversion table for COM baud rate */
	int x, mcr, oldpri;

	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id,tp)];
	flags = tp->t_cflag;

	x = (ICR_TIEN | ICR_SIEN | ICR_MIEN | ICR_RIEN);

	if (tp->t_cflag & CLOCAL)
		tp->t_state |= CARR_ON;
	else
		asyhpmodem(tp);

	outb(asyhp->asyhp_icr, x);

	x = inb(asyhp->asyhp_lcr);
	x &= ~(LCR_WLS0|LCR_WLS1|LCR_STB|LCR_PEN|LCR_EPS);
	if ((tp->t_cflag & CSTOPB) == CSTOPB )
		x |= LCR_STB;  

	if ((tp->t_cflag & CS6) == CS6 )
		x |= LCR_BITS6;
	if ((tp->t_cflag & CS7) == CS7 )
		x |= LCR_BITS7;
	if ((tp->t_cflag & CS8) == CS8 )
		x |= LCR_BITS8;
	if ((tp->t_cflag & PARENB) == PARENB )
		x |= LCR_PEN;
	if ((tp->t_cflag & PARODD) == 0)
		x |= LCR_EPS;
	outb(asyhp->asyhp_lcr, x);

	/*
	 * Set the baud rate:
	 * Bounds check baud rate index and then verify that the chip supports
	 * that speed.  Then program it. Default to 1200 baud.
	 */
	s = flags & CBAUD;
	if (s > MAXBAUDS || s < 0)
		s = B1200;

	if (s == 0) {
		/*
		 * only disable modem line
		 */
		if (!(tp->t_cflag&CLOCAL)) {
			(void) asyhpproc(tp, T_DISCONNECT);
			return(0);
		} else {
			return(EINVAL);
		}
	}

	oldpri = splstr();
	asyhp_prog(tp, s);
	splx(oldpri);

	return(0);
}


/*
 * called for Modem Status register changes while in DOSMODE.
 */
asyhpmodem(tp)
struct strtty *tp;
{
	register int msr;
	register struct msgb	*bp;
	struct asyhp *asyhpp;
	int	dev;
	extern asyhphangup();

	asyhpp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id,tp)];
	dev = (tp->t_dev / 2);

 	msr = inb(asyhpp->asyhp_msr);    /* this resets the interrupt */

	if (asyhpp->asyhp_flags & ASY82510 ) {
		outb(asyhpp->asyhp_msr,msr & 0xF0);
	}

 	if ( ( asyhpp->asyhp_flags & HWDEV) ) {
 		if( ( msr & MSR_CTS ) ) {
 			asyhpp->asyhp_flags |= HWFLWO ;
			asyhpproc(tp, T_RESUME);
 		} else {
 			asyhpp->asyhp_flags &= ~HWFLWO ;
			asyhpproc(tp, T_SUSPEND);
 
		}
	}

#ifdef VPIX
	if ((tp->t_iflag & DOSMODE) && (iasyintmask[dev] != V86VI_KBD)) {
		if (tp->t_iflag & PARMRK) {
			short c = (msr | (asyhpp->asyhp_dev<<8) | MSRWORD);

			if (tp->t_in.bu_ptr != NULL) {
				*(tp)->t_in.bu_ptr = c;
				(tp)->t_in.bu_cnt--;
				iasy_input(tp, L_BUF);
			}
		}

		if (iasystash[dev])
			if( validproc(iasy_v86_prp_pd[2*iasychan(dev)], 
							iasy_v86_prp_pd[(2*iasychan(dev)) + 1]))
					v86setint(iasystash[dev], iasyintmask[dev]);
			else {
					tp->t_iflag &= ~DOSMODE;
					iasystash[iasychan(dev)] = 0;
					iasyintmask[iasychan(dev)] = 0;
					iasy_v86_prp_pd[2*iasychan(dev)] = 0;
					iasy_v86_prp_pd[(2*iasychan(dev)) + 1] = 0;
			}
	}
	if (((tp->t_iflag & DOSMODE) && (iasyintmask[dev] == V86VI_KBD)) ||
						 !(tp->t_cflag & CLOCAL))
	{
#else
	if (!(tp->t_cflag & CLOCAL)) {
#endif /* VPIX */
		if (msr & MSR_DCD) {
			if (!(tp->t_state & CARR_ON))  {
				wakeup((caddr_t)&tp->t_rdqp);
				tp->t_state |= CARR_ON;
			}
		} else {
			if ((tp->t_state & CARR_ON) &&
                              (tp->t_state & ISOPEN)) {      
				iasy_hup(tp);
			}
			tp->t_state &= ~CARR_ON;
		}
	}
}


/*
 *	LOW LEVEL UTILITY ROUTINES
*/

/*
 * asyhpwakeup() -- release transmitter output.
 *
 * It is used by the TCSBRK ioctl command.  After .25 sec
 * timeout (see case BREAK in asyhpproc), this procedure is called.
 *
 */
void asyhpwakeup(tp)
struct strtty *tp;
{
	asyhpproc(tp, T_TIME);
}

/*
 *	asyhpSetDTR() -- assert the DTR line for this port
 */
asyhpSetDTR(asyhp)
struct asyhp *asyhp;
{
	int	mcr;

    mcr = inb(asyhp->asyhp_mcr);
    outb(asyhp->asyhp_mcr, (mcr|MCR_DTR));
}

/*
 * This procedure programs the baud rate generator.
 */
asyhp_prog(tp, speed)
struct strtty *tp;
ushort	speed;

{
	ushort y;
	unsigned char  x;
	register struct     asyhp *asyhp;

	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id,tp)];
	x = inb(asyhp->asyhp_lcr);
	outb(asyhp->asyhp_lcr, x|LCR_DLAB);
	y = asyhpspdtab[speed];
	outb(asyhp->asyhp_dat, y & 0xff);
	outb(asyhp->asyhp_icr, y >>8);
	outb(asyhp->asyhp_lcr, x);	
}

/*
 * This procedure does the initial reset on an COM USART.
 */
asyhp_reset(tp)
struct strtty *tp;

{
	register struct     asyhp *asyhp;
	unsigned char	flush_regs;

	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id,tp)];

	flush_regs = inb(asyhp->asyhp_isr);
	flush_regs = inb(asyhp->asyhp_lsr);
	flush_regs = inb(asyhp->asyhp_msr);
	flush_regs = inb(asyhp->asyhp_dat);
	
	outb(asyhp->asyhp_lcr, LCR_DLAB);
	outb(asyhp->asyhp_dat, asyhpspdtab[B1200] & 0xff);		
	outb(asyhp->asyhp_icr, asyhpspdtab[B1200]  >> 8);		
	outb(asyhp->asyhp_lcr, LCR_BITS8); 
	outb(asyhp->asyhp_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	
	tp->t_state &= ~BUSY;
}


/*
 * debugger/console support routines.
 */

/*
 * put a character out the first serial port.
 * Do not use interrupts.  If char is LF, put out LF, CR.
 */
void
asyhpputchar(c)
unsigned char	c;
{
	if (! asyhpinitialized)
		asyhpinit();
	if (inb(p_asyhp0+ISR) & 0x38)
		return;
	while((inb(p_asyhp0+LSR) & LSR_XHRE) == 0) /* wait for xmit to finish */
	{
		if ((inb(p_asyhp0+MSR) & MSR_DCD ) == 0)
			return;
		drv_usecwait(10);
	}
	outb(p_asyhp0+DAT, c); /* put the character out */
	if (c == '\n')
		asyhpputchar(0x0d);
}

/*
 * get a character from the first serial port.
 *
 * If no character is available, return -1.
 * Run in polled mode, no interrupts.
 */

int
asyhpgetchar()
{
	if ((inb(p_asyhp0+ISR) & 0x38) || (inb(p_asyhp0+LSR) & LSR_RCA) == 0) {
		drv_usecwait(10);
		return -1;
	}
	return inb(p_asyhp0+DAT);
}

#ifdef lint
/*
 *	Reference each routine that the kernel does, to keep lint happy.
*/
main()
{
asyhpinit();
asyhpintr(0);
return 0;
}
#endif /*lint*/
