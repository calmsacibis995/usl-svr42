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

#ident	"@(#)uts-x86at:io/asy/asyc.c	1.27"
#ident	"$header: $"

#ifndef lint
static char asyc_copyright[] = "Copyright 1991 Intel Corporation xxxxxx";
#endif /* lint */

/*
 * This driver forms the hardware dependent part of the serial driver.
 * It uses the iasy driver interface to comminicate to the modules upstream.
 *
 * It supports National 16450 and compatible chips, the lowest common
 * denominator UART.
 */

#include "util/types.h"
#include "svc/errno.h"
#include "util/cmn_err.h"
#include "io/stropts.h"
#include "proc/signal.h"
#include "util/param.h"
#include "io/conf.h"
#include "io/stream.h"
#include "io/termio.h"
#include "io/strtty.h"
#include "io/asy/iasy.h"
#include "io/asy/asyc.h"
#include "io/asy/asy.h"
#include "io/ddi.h"
#include "mem/kmem.h"
#include "io/tty.h"
#include "util/inline.h"
#ifdef VPIX
#include "proc/proc.h"
#include "proc/tss.h"
#include "vpix/v86.h"
#endif /* VPIX */


#ifdef DEBUGSTR
#include "io/strlog.h"
#include "io/log/log.h"
#define	DSTRLOG(x) strlog x 
#else
#define	DSTRLOG(x)
#endif /* DEBUGSTR */

extern	int	p_asyc0;
struct strtty *asyc_tty;
extern int asyc_cnt;			/* How many ports are configured */
int	asycproc();
int	asycgetchar();
void	asycputchar();
extern	int	asycgetchar2();
extern	void	asycputchar2();
void asycwakeup();

unsigned int asyc_alive=0;
unsigned int asycinitialized=0;
extern  struct  asyc asyctab[];   /* asyc structs for each port */
extern unsigned int asyc_num;
extern unsigned int asyc_sminor;
extern	int	iasy_console;
extern struct strtty asy_tty[];         /* iasy_tty changed to asy_tty for merge */

int	asyc_id;

#ifdef VPIX
extern v86_t *iasystash[];	/* save proc.p_v86 here for psuedorupts */
extern int iasyintmask[];	/* which pseudorupt to give */
extern struct termss iasyss[];	/* start/stop characters */
extern int   iasy_closing[];
extern char iasy_opened[];
extern int iasy_v86_prp_pd[];
extern  int     validproc();

int	asycvmproc();
#define MSRWORD FRERROR
#define LSRWORD PERROR
#endif /* VPIX */

#ifdef MERGE386
extern  int     asy_is_assigned();
extern  int     com_ppiioctl();
extern  int     merge386enable;

#endif /* MERGE386 */

/*
 * Baud rate table. Indexed by #defines found in io/termios.h
 */

#define MAXBAUDS 17
ushort asycspdtab[] = {
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

#ifdef INEXT
#undef INEXT
#endif /* INEXT */
#define	ONEXT(x) (((x) < (ushort)(OSIZE-1)) ? ((x) + 1) : 0)
#define	INEXT(x) (((x) < (ushort)(ISIZE-1)) ? ((x) + 1) : 0)

/*
 * asycinit() -- initialize driver with interrupts off
 *
 *  This is called before the interrupts are turned on to initalize
 *  the device.  Reset the puppy and program it do async IO and to
 *  give interrupts for input and output.
 */
void
asycinit()
{	
	int	unit;
	struct asyc *asyc;
	int	size;
	char	*bufp;
	extern	void asycpoll();
	extern	int timeout();
	extern	int asyc_bufp[];
	int	(*vmproc)() = 0;	/* VPIX/MERGE support func. ptr */

	if (asycinitialized)
		return;
	asycinitialized=1;

	for (unit=0; unit<asyc_num; unit++) {
		asyc = &asyctab[unit];
		/* 
		 * Bit 4 & 5 of ISR are wired low. If bit 4 or 5 appears on inb(),
		 * board is not there.
		 */ 
		if ((inb(asyc->asyc_isr) & 0x30)) {
			asyc->asyc_flags &= ~ASYHERE;
			continue; /* no serial adapter here */
		}
		asyc->asyc_flags |= ASYHERE;

		/*
		 * Reset all registors for soft restart.
		 */

		outb(asyc->asyc_icr,0x00);
		outb(asyc->asyc_isr,0x00);
		outb(asyc->asyc_lcr,0x00);
		outb(asyc->asyc_mcr,0x00);


		outb(asyc->asyc_isr,0x20);
		if (inb(asyc->asyc_isr) & 0x20) {
			asyc->asyc_flags |= ASY82510; /* 82510 chip present */
			outb((asyc->asyc_dat + 0x7),0x04);   /* Status clear */
			outb(asyc->asyc_isr,0x40);       /* set to bank 2 */
			outb(asyc->asyc_mcr,0x08); /*  IMD */
			outb(asyc->asyc_dat,0x21); /*  FMD */
			outb(asyc->asyc_isr,0x00);       /* set to bank 0 */
		}

		++asyc_alive;
	}
	
	if (!asyc_alive)
		return;

	/*	Assign interrupt level buffers	*/
	bufp = (char *)asyc_bufp;
	for (unit=0, asyc=&asyctab[unit]; unit < asyc_num; unit++, asyc++) {
		asyc->bp = (struct asyc_aux *)bufp;
		asyc->bp->oput=asyc->bp->oget=asyc->bp->iget=asyc->bp->iput=0;
		asyc->bp->asyc_state = 0;
		asyc->bp->asyc_iflag = 0;
		asyc->bp->fifo_size = 1;

		/* Set each UART in FIFO mode */

		if( (asyc->asyc_flags & ASY82510) == 0 ) {
			outb(asyc->asyc_isr, 0xc1);	/* check for 550 */
			if ((inb(asyc->asyc_isr) & 0xc0) == 0xc0){ /* 16550 ? */
				asyc->asyc_flags |= ASY16550;
				asyc->bp->fifo_size = 16;
			
				outb(asyc->asyc_isr, 0x0); /*  clear both fifos */
				outb(asyc->asyc_isr, FIFOEN550); /* fifo trigger for 16550 chip */
			/*
			}else{
				outb(asyc->asyc_isr, 0x0);  clear both fifos 
				outb(asyc->asyc_isr, FIFOEN450);  fifo trigger set for 16450	 
			*/
			}
		}
		/*
		 * Reset all interrupts for soft restart.
		 */

		(void)inb(asyc->asyc_isr);
        	(void)inb(asyc->asyc_lsr);
        	(void)inb(asyc->asyc_msr);
        	(void)inb(asyc->asyc_dat);

		bufp += sizeof (struct asyc_aux);	
	}

#ifdef	VPIX
	vmproc = asycvmproc;
#else
	vmproc = (int (*)())NULL;
#endif

	/* Register terminal server. */
	if ((asyc_sminor == 0) && (iasy_console == 0)){
		asyc_id = iasy_register(asyc_sminor, asyc_num, asycproc, iasyhwdep, asycgetchar, asycputchar, vmproc);
	}else{
		if ((asyc_sminor <= iasy_console) && (iasy_console <= asyc_num) && (iasy_console == 1)){
			asyc_id = iasy_register(asyc_sminor, asyc_num, asycproc, iasyhwdep, asycgetchar2, asycputchar2, vmproc);
		}else{
			asyc_id = iasy_register(asyc_sminor, asyc_num, asycproc, iasyhwdep,0, 0, vmproc);
		}
	}
	asyc_tty = IASY_UNIT_TO_TP(asyc_id, 0);
	timeout(asycpoll, (caddr_t)0, drv_usectohz(20000));
}

/*
 *	INTERRUPT TIME EXECUTION THREAD
*/
/*
 * asycintr() -- process device interrupt
 *
 * This procedure is called by system with interrupts off
 * when the USART interrupts the processor. 
 *
 */
/*ARGSUSED*/
void
asycintr(vect)
int	vect;			/* Unused */
{	
	register struct	strtty *tp;	/* strtty structure */
	register	unit;
	struct asyc *asyc;
	uchar_t		c;
	unsigned char   interrupt_id, line_status, modem_status;
	struct	asyc_aux	*ap;
	void	asycProcessInChar();
	void	asycstartio();
	void	asycmodem();

    for (asyc = &asyctab[0], unit =0;  unit < asyc_num ; asyc++, unit++)
        if( (asyc->asyc_vect == vect) && ( asyc->asyc_flags & ASYHERE))
            break;

    if (unit >= asyc_num)
        return;

#ifdef MERGE386
	/* needed for com port attachment */
        if(merge386enable)
                if (asyc->asyc_ppi_func && (*asyc->asyc_ppi_func) (asyc, -1))
                        return;

#endif /* MERGE386 */

	for (;;) {
nextloop:
		interrupt_id =   inb(asyc->asyc_isr) & 0x0f; 
		line_status  =   inb(asyc->asyc_lsr);
/*
		modem_status =   inb(asyc->asyc_msr);
*/
	
		tp = IASY_UNIT_TO_TP(asyc_id, unit);
		ap=asyctab[unit].bp;

		if ((interrupt_id == ISR_RxRDY) || (interrupt_id == ISR_FFTMOUT) || 
					(line_status == LSR_RCA) || (interrupt_id == ISR_RSTATUS)) {
	
			while (line_status & LSR_RCA){
				drv_setparm(SYSRINT, 1);
				c = inb(asyc->asyc_dat) & 0xff;
			
				if ( (tp->t_cflag & CREAD) != CREAD ){
					goto	nextloop;
				}

#ifdef MERGE386
				/* Needed for direct attachment of the serial port */
				if(merge386enable) {
					if (asyc->asyc_ppi_func && (*asyc->asyc_ppi_func) (asyc, c)) {
						goto	nextloop;
					}
				}
#endif /* MERGE386 */

				asycProcessInChar(tp, c, line_status);	
				line_status = inb(asyc->asyc_lsr);
			} 
		} else
		if (interrupt_id == ISR_TxRDY || (line_status & LSR_XHRE) &&
						(ap->asyc_state & BUSY)) {
			drv_setparm(SYSXINT, 1);
			ap->asyc_state &= ~BUSY;
			asycstartio(tp);
		} else
		if (interrupt_id == ISR_MSTATUS)  
			asycmodem(tp);
		else
			return;
	}
}

/*
 * Called at spltty from the interrupt thread.
 *
 * asycProcessInChar() -- do the input processing for a character
 * This driver is for basic USARTs that don't have any on-chip storage.
 * So in order to ensure we don't lose any character, the input logic below
 * simply stuffs the character into the driver input buffer for the specific 
 * device. The processing of the character and sending it upstream is taken
 * care of in asycpoll().
*/
void
asycProcessInChar(tp, c, line_status)
struct	strtty	*tp;
uchar_t	c;
uchar_t	line_status;
{
	register struct asyc_aux *ap;
	struct	asyc	*asyc;
	unsigned short	ierr=0;
	int	dev;
	void	asycstartio();

	dev = tp->t_dev / 2;
        asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];
        ap = asyc->bp;


	if (!(tp->t_state & ISOPEN))
			return;

	if ((ap->asyc_iflag & IXON) && !(asyc->asyc_flags & HWDEV)) {
		/* if stopped, see if to turn on output */
#ifdef VPIX
		if (ap->asyc_state & TTSTOP) {
			if (c == iasyss[dev].ss_start ||
						ap->asyc_iflag&IXANY) {
				ap->asyc_state &= ~TTSTOP;
				tp->t_state &= ~TTSTOP;
				asycstartio(tp);
			}
		} else {
			/* maybe we're supposed to stop output */
			if (c == iasyss[dev].ss_stop && (c != 0)) {
				ap->asyc_state |= TTSTOP;
				tp->t_state |= TTSTOP;
			}
		}
		if ((c != 0) && ((c == iasyss[dev].ss_stop) || (c == iasyss[dev].ss_start)))
			return;
#else
		if (ap->asyc_state & TTSTOP) {
			if (c == tp->t_cc[VSTART] ||
						ap->asyc_iflag&IXANY) {
				ap->asyc_state &= ~TTSTOP;
				tp->t_state &= ~TTSTOP;
				asycstartio(tp);
			}
		} else {
			/* maybe we're supposed to stop output */
			if (c == tp->t_cc[VSTOP] && (c != 0)) {
				ap->asyc_state |= TTSTOP;
				tp->t_state |= TTSTOP;
			}
		}
		if ((c != 0) && ((c == tp->t_cc[VSTOP]) || (c == tp->t_cc[VSTART])))
			return;
#endif
	}

	if (INEXT(ap->iput) != ap->iget) {
		if (line_status & (LSR_PARERR|LSR_FRMERR|LSR_BRKDET|LSR_OVRRUN)) {
#ifdef VPIX
			if ((tp->t_iflag & (DOSMODE | PARMRK)) == 
									(DOSMODE | PARMRK)){
				DSTRLOG((16450, 0, 0, SL_TRACE, 
								"asycProcessInChar: DOSMODE|PARMRK"));
				ierr = (line_status | LSRWORD);
				line_status=0;
			}
#endif /* VPIX */
			if (line_status & LSR_PARERR)
				ierr = PERROR;
			else if (line_status & LSR_FRMERR|LSR_BRKDET)
				ierr = FRERROR;
			else if (line_status & LSR_OVRRUN)
				ierr = OVERRUN;
		}

		ap->ierrs[ap->iput] = (ierr>>8) & 0xff;
		ap->ibuf[ap->iput] = c;
		ap->iput=INEXT(ap->iput);

		/* New IXOFF at interrupt level */
		if ((ap->asyc_iflag & IXOFF) && !(ap->asyc_state & TBLOCK)) {
			int	lcnt = ap->iput - ap->iget;
			if (lcnt < 0)
				lcnt +=ISIZE;
			if (lcnt >= (ISIZE*3/4)) {
				ap->asyc_state |= TTXOFF|TBLOCK;
				ap->asyc_state &= ~TTXON;
			}
		}
	}
	asycstartio(tp);
}

/*
 * asycpoll() loops for all the devices, checking if there is input in the
 * driver buffers (asyc_aux) put there by the interrupt routine. If so, it
 * does any needed processing and calls iasy_input() to ship the characters
 * upstream. 
 * Also, for each device, if there is anything in the tty output buffer (t_out)
 * put there by iasy_output(), it transfers them to the driver output buffer
 * to be processed by asycstartio().
 */

/*ARGUSED*/
void
asycpoll(dummy)
int dummy;
{
	register struct strtty *tp;
	register struct asyc_aux *ap;
	int dev, unit, s;
	short lcnt;
	uchar_t lbuf[3], c;
	unsigned short stat;
	boolean_t asyc_input_flag;
	int old_pri;
	void	asycstartio();
	int	spltty(), splx();
	int	timeout();

	old_pri = splstr();

	for (unit=0, tp=asyc_tty, ap = asyctab[unit].bp; unit < asyc_num; 
							unit++, tp++, ap++) {
		if (!(asyctab[unit].asyc_flags & ASYHERE))
			continue;
		s = spltty();
		if (!(tp->t_state & ISOPEN)) {
			ap->asyc_state &= ~ISOPEN;
			splx(s);
			continue;
		}

		asyc_input_flag = B_FALSE;

		dev = tp->t_dev / 2;

		if (ap->iget != ap->iput) {
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
			if ((ap->asyc_iflag & IXOFF) && (ap->asyc_state & TBLOCK)) {
				lcnt = ap->iput - ap->iget;
				if (lcnt < 0) 
					lcnt += ISIZE;
				if (lcnt < (ISIZE/4)) {
					ap->asyc_state &= ~(TTXOFF|TBLOCK);
					ap->asyc_state |= TTXON;
					asycstartio(tp);
				}
			}
		}

		/* input ready at interrupt level? */
		while (ap->iget != ap->iput) {

			if (tp->t_state & TBLOCK) 
				break;
			if (tp->t_in.bu_cnt < 3){
				splx(s);
                                if (!(iasy_input(tp, L_BUF))){
                                        asyc_input_flag = B_FALSE;
                                        s=spltty();
                                }else{
                                        s=spltty();
                                        break;
                                }
			}
			lcnt = 1;
			c = ap->ibuf[ap->iget];
			stat = (ap->ierrs[ap->iget] << 8) & 0xff00;

			if (stat & PERROR && !(tp->t_iflag & INPCK)) {
				if( (tp->t_cflag & (PARODD|PARENB)) != (PARODD|PARENB)
					&& (( c & 0377 ) != 0 ))
						stat &= ~PERROR;
			}

			if (stat & (FRERROR|PERROR|OVERRUN)) {
#ifdef VPIX
				if ((tp->t_iflag & (DOSMODE | PARMRK)) ==
											(DOSMODE | PARMRK)) {
					DSTRLOG((16450, 0, 0, SL_TRACE, 
								"asycpoll: DOSMODE|PARMRK"));
					lcnt=3;
					lbuf[2]=0377;	
					if (c & MSRWORD)
							lbuf[1]=2;
					else if (c & LSRWORD)
							lbuf[1]=1;
					lbuf[0]=(char)c;
				}
#endif /* VPIX */
				if ((c & 0377) == 0) {
					if (!(tp->t_iflag & IGNBRK)) {
						ap->iget = INEXT(ap->iget);
						splx(s);
						if (asyc_input_flag == B_TRUE) {
							/* Ship this upstream first. */
							if (!(iasy_input(tp, L_BUF))){
								asyc_input_flag = B_FALSE;
							}
						}
						iasy_input(tp, L_BREAK);
						s=spltty();
						break;
					}else{
						ap->iget = INEXT(ap->iget);
						continue;
					}
		            	} else {
					if ((tp->t_iflag & IGNPAR)) {
						/* If IGNPAR is set, characters with framing and
						   parity errors are ignored (except BREAK) */
						ap->iget = INEXT(ap->iget);
	                    			continue;
					}
				}
				if (tp->t_iflag & PARMRK) {
					/* Mark the parity errors */

					lcnt = 3;
					lbuf[2] = (unchar)0377;	
					lbuf[1] = 0;
				}else
					/* Send up a NULL character */
					c = 0;
			}
			else{
				if (tp->t_iflag & ISTRIP) {
					c &= 0177;
				} else {
					if (c == 0377 && tp->t_iflag&PARMRK) {
						/* if marking parity errors, this character gets doubled */
						lcnt=2;
						lbuf[1]=c;
					}
				}

			}
			lbuf[0]=c;

			/*
			 *In the couple of pieces of code below, we are trying to 
			 * optimize by calling iasy_input when the input buffer is full.  
			 * The asyc_input_flag is there to keep track of anytime anything is
			 * put into the input buffer for the current tty.  After exiting 
			 * this for loop, we still want to ship whatever is in the buffer 
			 * upstream.  We could call iasy_input everytime, even if there is
			 * nothing in the buffer, but it is more expensive.  So we only
			 * call it if something is there to send.
			*/
			if (tp->t_in.bu_ptr != NULL) {
				while (lcnt > 0) {
					*(tp->t_in.bu_ptr++) = lbuf[--lcnt];
					asyc_input_flag = B_TRUE;	/* something is in the buffer */
					tp->t_in.bu_cnt--;
					if (tp->t_in.bu_cnt == 0) {
						splx(s);
						if (!(iasy_input(tp, L_BUF))){
							asyc_input_flag = B_FALSE;	/* nothing in the buffer */
							s = spltty();
						}else{
							s = spltty();
							break;
						}
					}
				}
			}
			ap->iget = INEXT(ap->iget);
		} /* while ap->iget != ap->iput */
		splx(s);

		/*
		 *	Of course, after exiting the for loop above, we want to ship 
		 *  anything else that might be in the input buffer right away.
		*/

		if (tp->t_in.bu_cnt != IASY_BUFSZ){
			iasy_input(tp, L_BUF);
		}

		/* 
		**	copy output to interrupt level buffer
		*/
		s = spltty();
		if (!(ap->asyc_state & BUSY))
			tp->t_state &= ~BUSY;
		splx(s);
		if (!(tp->t_out.bu_ptr && tp->t_out.bu_cnt)) {
			if (!(CPRES & iasy_output(tp))) {
				s = spltty();
				if (ap->oput != ap->oget){
                			if (!(ap->asyc_state & BUSY))
                        			tp->t_state &= ~BUSY;
					if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) == 0){
						asycstartio(tp);
						if (ap->asyc_state & BUSY)
                        				tp->t_state |= BUSY;
						splx(s);
						continue;
					}
				}
				splx(s);
				continue;
			}
		}
		s = spltty();
		while (ONEXT(ap->oput) != ap->oget) {
			ap->obuf[ap->oput] = *tp->t_out.bu_ptr++;
			ap->oput = ONEXT(ap->oput);
			tp->t_out.bu_cnt--;
			if (tp->t_out.bu_cnt == 0){
				if (!(CPRES & iasy_output(tp))) {
					break;
				}
			}
		} 	/* while there are characters to output */
		if (!(ap->asyc_state & BUSY))
			tp->t_state &= ~BUSY;
		if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) != 0){
			splx(s);
			continue;
		}
		asycstartio(tp);
		if (ap->asyc_state & BUSY)
			tp->t_state |= BUSY;
		splx(s);
	} /* for all ports */
	timeout(asycpoll, (caddr_t)0, drv_usectohz(20000));
	splx(old_pri);
}

#ifdef VPIX
int 
asycvmproc(tp, cmd, argp1, argp2)
struct strtty *tp;
int	cmd;
char *argp1;
int *argp2;
{
	int 	s, rq;
	struct asyc *asyc;
	char 	val, val2;
	int	efl;
	int	ret_val=0;

	s = spltty();
    /*
     * get device number and control port
     */
    	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];;
	DSTRLOG((16450, 0, 0, SL_TRACE, "asycvmproc 0x%x", cmd));
	switch (cmd) {
#ifdef MERGE386
		case COMPPIIOCTL: /* Do com_ppiioctl() */
			ret_val = com_ppiioctl( OTHERQ(tp->t_rdqp), (mblk_t *)argp1, 
										&asyctab[tp->t_dev/2], (*argp2));
			break;
#endif /* MERGE386 */
		case AIOCDOSMODE:
			val = inb(asyc->asyc_icr ) | ICR_MIEN;
			outb(asyc->asyc_icr , val);
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
				val2 = inb(asyc->asyc_lcr);
				outb(asyc->asyc_lcr, val2 | LCR_DLAB);
				outb(asyc->asyc_dat, val);
				outb(asyc->asyc_lcr, val2 & ~LCR_DLAB);
				intr_restore(efl);
			}
			if (rq & SIO_MASK(SO_DIVLMSB)) {
				val = fubyte(argp1+SO_DIVLMSB);
				efl = intr_disable();
				val2 = inb(asyc->asyc_lcr);
				outb(asyc->asyc_lcr, val2 | LCR_DLAB);
				outb(asyc->asyc_icr, val);
				outb(asyc->asyc_lcr, val2 & ~LCR_DLAB);
				intr_restore(efl);
			}
			if (rq & SIO_MASK(SO_LCR)) {
				val = fubyte(argp1+SO_LCR);
				outb(asyc->asyc_lcr, val);
			}
			if (rq & SIO_MASK(SO_MCR)) {
				val = fubyte(argp1+SO_MCR);
				/* force OUT2 on to preserve interrupts */
				outb(asyc->asyc_mcr, val | MCR_OUT2);
			}
			break;
	    case AIOCSERIALIN:
			rq = fubyte(argp1);
			if (rq & SIO_MASK(SI_MSR)) {
				subyte(argp1+SI_MSR,inb(asyc->asyc_mcr));
			}
			break;
		default:
			cmn_err(CE_NOTE, "asycvmproc: Unknown command");
			break;
	}
	splx(s);
	return(ret_val);
}
#endif /* VPIX */

/*
 * asycproc() -- low level device dependant operations
 *
 * It is called at both task time by the line discipline routines,
 * and at interrupt time by asycintr().
 * asycproc handles any device dependent functions required
 * upon suspending, resuming, blocking, or unblocking output; flushing
 * the input or output queues; timing out; sending break characters,
 * or starting output.
 */
int
asycproc(tp, cmd)
register struct strtty *tp;
int	cmd;
{
	int s;
	unsigned char line_ctl;
	int	dev;
	struct asyc *asyc;
	int	ret=0;
	struct	asyc_aux *ap;
	void	asycstartio();
	int	asycparam();
	void	asycsetdtr();
	void	asycu_reset();
	int	timeout();
	void	wakeup();

    	dev = tp->t_dev / 2;
    	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];
	ap=asyc->bp;

	s=spltty();

	switch (cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
        	line_ctl = inb(asyc->asyc_lcr);
        	outb( asyc->asyc_lcr, line_ctl & ~LCR_SETBREAK );
		asycstartio(tp);
		break;

	case T_WFLUSH:
		tp->t_out.bu_cnt = 0;   /* abandon this buffer */
		ap->oput = ap->oget;
		break;

	case T_RESUME:
/*
 		if ( (asyc->asyc_flags & (HWDEV | HWFLWO) == (HWDEV | HWFLWO) ) ){
			break;
		}
*/

		tp->t_state &= ~TTSTOP;
		ap->asyc_state &= ~TTSTOP;
		asycstartio(tp);
		break;

	case T_SUSPEND:
		tp->t_state |= TTSTOP;
		ap->asyc_state |= TTSTOP;
		break;

	case T_RFLUSH:
		ap->iget = ap->iput;
		tp->t_in.bu_cnt = IASY_BUFSZ;
		tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		/* FALLTHRU */

	case T_UNBLOCK:
		tp->t_state &= ~TBLOCK;
		if (ap->asyc_state & TBLOCK) {
			ap->asyc_state &= ~(TTXOFF|TBLOCK);
			ap->asyc_state |= TTXON;
			asycstartio(tp);
		}
		break;

	case T_BLOCK:
		tp->t_state |= TBLOCK;
		ap->asyc_state &= ~TTXON;
		ap->asyc_state |= TBLOCK|TTXOFF;
		asycstartio(tp);
		break;

	case T_BREAK:
		tp->t_state |= TIMEOUT;
        	line_ctl = inb(asyc->asyc_lcr);
        	outb(asyc->asyc_lcr, line_ctl | LCR_SETBREAK);
		(void) timeout(asycwakeup, (caddr_t)tp, HZ/4);
		break;

	case T_OUTPUT:
		/*
		 **
                 */
		splx(s);
                if (!(tp->t_out.bu_ptr && tp->t_out.bu_cnt)) {
                        if (!(CPRES & iasy_output(tp))) {
				s = spltty();
				if (ap->oput != ap->oget){
                			if (!(ap->asyc_state & BUSY))
                        			tp->t_state &= ~BUSY;
					if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) == 0){
						asycstartio(tp);
						if (ap->asyc_state & BUSY)
                        				tp->t_state |= BUSY;
						break;
					}
				}
				break;
                        }
                }
		s = spltty();
                while (ONEXT(ap->oput) != ap->oget) {
                        ap->obuf[ap->oput] = *tp->t_out.bu_ptr++;
                        ap->oput = ONEXT(ap->oput);
                        tp->t_out.bu_cnt--;
                        if (tp->t_out.bu_cnt == 0){
                        	if (!(CPRES & iasy_output(tp))) {
                                	break;
                		}
			}

                }      
                if (!(ap->asyc_state & BUSY))
                        tp->t_state &= ~BUSY;

		if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) != 0)
			break;

		asycstartio(tp);
		if (ap->asyc_state & BUSY)
                        tp->t_state |= BUSY;
		break;

	case T_CONNECT:

#ifdef MERGE386
        if (asyc->asyc_ppi_func && asy_is_assigned(asyc) ) {
                ret = EACCES;
				break;
        }
#endif /* MERGE386 */

		if (!(asyc->asyc_flags & ASYHERE)) {
			ret=ENXIO;
			break;
		}

		if (asyc->asyc_flags & ASY82510)
			outb(asyc->asyc_isr, 0x0);
		if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {
			
			if( (asyc->asyc_flags & ASY82510) == 0 ) {
				outb(asyc->asyc_isr, 0x0);  /* clear both fifos */
				if (asyc->asyc_flags & ASY16550){
					outb(asyc->asyc_isr, FIFOEN550); /* fifo trigger for 16550 chip */
				/*
				}else{
					outb(asyc->asyc_isr, FIFOEN450);  fifo trigger set for 16450 
				*/
				}
			}

			/*
			outb(asyc->asyc_lcr, LCR_DLAB);
			outb(asyc->asyc_dat, asycspdtab[B9600] & 0xff);		
			outb(asyc->asyc_icr, asycspdtab[B9600]  >> 8);		
			outb(asyc->asyc_lcr, LCR_BITS8); 
			outb(asyc->asyc_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	
			*/

			asycu_reset(tp);
			asycparam(tp);
			ap->iget=ap->iput=ap->oget=ap->oput=0;
			ap->asyc_state = ISOPEN;
			if (tp->t_dev % 2) {
				asyc->asyc_flags |= HWDEV;
				asyc->asyc_flags &= ~(HWFLWO);
			}
			else {
				asyc->asyc_flags &= ~(HWDEV|HWFLWO);
			}
		}

		asycsetdtr(asyc);

		if(tp->t_cflag & CLOCAL)
			tp->t_state |= CARR_ON;
		else {
			if (inb(asyc->asyc_msr) & MSR_DCD) {
				tp->t_state |= CARR_ON;
				iasy_carrier(tp);
			} else
				tp->t_state &= ~CARR_ON;
		}     
		if (tp->t_dev % 2) {
			if (inb(asyc->asyc_msr) & MSR_CTS) {
				asyc->asyc_flags |= HWFLWO;
			}else{
				asyc->asyc_flags &= ~HWFLWO;
			}
		}
		break;

	case T_DISCONNECT:
		outb (asyc->asyc_mcr, (MCR_RTS | MCR_OUT2));
		break;

	case T_PARM:
		if (!(inb(asyc->asyc_lsr) & LSR_XSRE)){
			/* Wait for one character time for Transmitter shift 
				register to get empty	*/
			ret = iasy_ctime(tp, 1);
		}else{
			ret = asycparam(tp);
		}
		break;
	case T_TRLVL1:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL1);
		break;
	case T_TRLVL2:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL2);
		break;
	case T_TRLVL3:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL3);
		break;
	case T_TRLVL4:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL4);
		break;
	}
	splx(s);
	return (ret);
}

/*
 * Called at spltty.
 *
 * asycstartio() -- start output on an serial channel if needed.
 *
 * Get a character from the character queue, output it to the
 * channel and set the BUSY flag. The BUSY flag gets reset by asycintr
 * when the character has been transmitted.
 */
void
asycstartio(tp)
struct	strtty	*tp;
{
	struct	asyc	*asyc;
	struct  asyc_aux *ap;
	int		dev;
	int		c;
	ushort	charcount = 0;

	dev = tp->t_dev / 2;
	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];
	ap = asyc->bp;

	if (ap->asyc_state & (BUSY|TTSTOP)) /* Wait for a better time. */
		return;
	if ((asyc->asyc_flags & (HWDEV | HWFLWO)) == HWDEV)
		return;
	
	if (ap->asyc_state & (TTXON|TTXOFF)) {
		if (ap->asyc_state & TTXON) {
			ap->asyc_state &= ~TTXON;
 			if ( asyc->asyc_flags & HWDEV ) {
				char mcr = inb(asyc->asyc_mcr);
				outb(asyc->asyc_mcr, mcr | MCR_RTS);
 			}else{ 
#ifdef VPIX
				outb(asyc->asyc_dat, iasyss[dev].ss_start);
#else
 				outb(asyc->asyc_dat, tp->t_cc[VSTART]);
#endif /* VPIX */
				charcount++;
				ap->asyc_state |= BUSY;
			}
		} else {
			ap->asyc_state &= ~TTXOFF;
                        if ( asyc->asyc_flags & HWDEV ) {
                                char mcr = inb(asyc->asyc_mcr);
                                outb(asyc->asyc_mcr, mcr & ~MCR_RTS);
			}else{
#ifdef VPIX
 				outb(asyc->asyc_dat, iasyss[dev].ss_stop);
#else
		 		outb(asyc->asyc_dat, tp->t_cc[VSTOP]);
#endif /* VPIX */
				charcount++;
				ap->asyc_state |= BUSY;
			}
		}
	}

	while (charcount < ap->fifo_size ){
		if (ap->oput == ap->oget)
			return;
	
		ap->asyc_state |= BUSY;
		c=ap->obuf[ap->oget];
		outb(asyc->asyc_dat, c);
		ap->oget=ONEXT(ap->oget);

		charcount++;
	}
}


/*
 * Called at spltty.
/* Param detects modifications to the line characteristics and reprograms 
 */
asycparam(tp)
struct	strtty	*tp;
{
	struct asyc *asyc;
	unsigned int flags;  /* To ease access to c_flags    */
	int s;      /* Index to conversion table for COM baud rate */
	int x;
	void	asyct_prog();
	void	asycmodem();

	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];
	flags = tp->t_cflag;
	x = (ICR_TIEN | ICR_SIEN | ICR_MIEN | ICR_RIEN);

	if (tp->t_cflag & CLOCAL)
		tp->t_state |= CARR_ON;
	else
		asycmodem(tp);

	outb(asyc->asyc_icr, x);

	x = inb(asyc->asyc_lcr);
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
	outb(asyc->asyc_lcr, x);

	/*
	 * Set the baud rate:
	 * Bounds check baud rate index and then verify that the chip supports
	 * that speed.  Then program it. Default to 9600 baud.
	 */
	s = flags & CBAUD;
	if (s > MAXBAUDS || s < 0)
		s = B9600;

	if (s == 0) {
		/*
		 * only disable modem line
		 */
		if (!(tp->t_cflag&CLOCAL)) {
		/*	iasy_hup(tp);	*/
			(void) asycproc(tp, T_DISCONNECT);
			return(0);
		} else {
			return(EINVAL);
		}
	}

	asyct_prog(tp, s);

	asyc->bp->asyc_iflag &= ~(IXON|IXANY|IXOFF);
	asyc->bp->asyc_iflag |= (tp->t_iflag & (IXON|IXANY|IXOFF));

	return(0);
}


/*
 * Called at spltty.
 *
 * called for Modem Status register changes while in DOSMODE.
 */
void
asycmodem(tp)
struct	strtty	*tp;
{
	register int msr;
	int	dev;
	struct asyc *asycp;
	struct	asyc_aux	*ap;
	int		timeout();
	void	wakeup();

	dev = tp->t_dev / 2;
	asycp = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];
	ap = asycp->bp;

 	msr = inb(asycp->asyc_msr);    /* this resets the interrupt */

	if (asycp->asyc_flags & ASY82510 ) {
		outb(asycp->asyc_msr,msr & 0xF0);
	}

	if(!(tp->t_state & (ISOPEN|WOPEN))) 
		return;

 	if ( ( asycp->asyc_flags & HWDEV) ) {
 		if( ( msr & MSR_CTS ) ) {
/*
			(void)asycproc(tp, T_RESUME);
*/
 			asycp->asyc_flags |= HWFLWO ;
			asycstartio(tp);
 		} else {
 			asycp->asyc_flags &= ~HWFLWO ;
/*
			(void)asycproc(tp, T_SUSPEND);
*/
 
		}
	}

#ifdef VPIX
	if ((tp->t_iflag & DOSMODE) && (iasyintmask[dev] != V86VI_KBD)) {
		if (tp->t_iflag & PARMRK) {
			ap->ibuf[ap->iput] = (char)msr;
			ap->ierrs[ap->iput] = (asycp->asyc_dev | (MSRWORD >> 8)) & 0xff;
			ap->iput = INEXT(ap->iput);
		}

		if (iasystash[(dev)])
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
				tp->t_state   |= CARR_ON;
			}
		}
		else {
 			if ((tp->t_state & CARR_ON) &&
                              (tp->t_state & ISOPEN)) {
/* 			if (tp->t_state & CARR_ON) { */
				tp->t_state   &= ~CARR_ON;
				timeout(iasy_hup,tp,1);
			}
		}
	}
}


/*
 *	LOW LEVEL UTILITY ROUTINES
*/
/*
 * asycwakeup() -- release transmitter output.
 *
 * It is used by the TCSBRK ioctl command.  After .25 sec
 * timeout (see case BREAK in asycproc), this procedure is called.
 *
 */
void asycwakeup(tp)
struct strtty *tp;
{
	asycproc(tp, T_TIME);
}

/*
 *	asycsetdtr() -- assert the DTR line for this port
 */
void
asycsetdtr(asyc)
struct asyc *asyc;
{
	int	mcr;

    mcr = inb(asyc->asyc_mcr);
    outb(asyc->asyc_mcr, (mcr|MCR_DTR));
}

/*
 * This procedure programs the baud rate generator.
 */
void
asyct_prog(tp, speed)
struct	strtty	*tp;
ushort	speed;

{
	ushort y;
	unsigned char  x;
	register struct     asyc *asyc;

	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];

	x = inb(asyc->asyc_lcr);
	outb(asyc->asyc_lcr, x|LCR_DLAB);
	y = asycspdtab[speed];
	outb(asyc->asyc_dat, y & 0xff);
	outb(asyc->asyc_icr, y >>8);
	outb(asyc->asyc_lcr, x);	
}

/*
 * This procedure does the initial reset on an COM USART.
 */
void
asycu_reset(tp)
struct	strtty	*tp;
{
	register struct     asyc *asyc;

	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id,tp)];

	(void)inb(asyc->asyc_isr);
	(void)inb(asyc->asyc_lsr);
	(void)inb(asyc->asyc_msr);
	(void)inb(asyc->asyc_dat);
	
	outb(asyc->asyc_lcr, LCR_DLAB);
	outb(asyc->asyc_dat, asycspdtab[B9600] & 0xff);		
	outb(asyc->asyc_icr, asycspdtab[B9600]  >> 8);		
	outb(asyc->asyc_lcr, LCR_BITS8); 
	outb(asyc->asyc_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	

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
asycputchar(c)
unsigned char	c;
{
	if (! asycinitialized)
		(void)asycinit();
	if (inb(p_asyc0+ISR) & 0x38)
		return;
	while((inb(p_asyc0+LSR) & LSR_XHRE) == 0) /* wait for xmit to finish */
	{
		if ((inb(p_asyc0+MSR) & MSR_DCD ) == 0)
			return;
		drv_usecwait(10);
	}
	outb(p_asyc0+DAT, c); /* put the character out */
	if (c == '\n')
		asycputchar(0x0d);
}

/*
 * get a character from the first serial port.
 *
 * If no character is available, return -1.
 * Run in polled mode, no interrupts.
 */

int
asycgetchar()
{
	if ((inb(p_asyc0+ISR) & 0x38) || (inb(p_asyc0+LSR) & LSR_RCA) == 0) {
		drv_usecwait(10);
		return -1;
	}
	return inb(p_asyc0+DAT);
}

#ifdef lint
/*
 *	Reference each routine that the kernel does, to keep lint happy.
*/
main()
{
	(void)asycinit();
	asycintr(0);
	return 0;
}
#endif /*lint*/
