/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/ws/ws_8042.c	1.14"
#ident	"$Header: $"


#include <util/types.h>
#include <util/debug.h>
#include <io/kd/kd.h>
#include <io/kd/kb.h>
#include <io/ws/8042.h>
#include <svc/bootinfo.h>
#include <util/cmn_err.h>
#include <util/inline.h>

#define AUX_DISAB	0x20
#define	KBD_DISAB	0x10

struct i8042 {
	int	s_spl;		/* saved spl level */
	unchar	s_state,	/* state of 8042 */
		s_saved,	/* indicates data was saved */
		s_data,		/* saved data (scan code or 320 mouse input) */
		s_dev2;		/* device saved character is meant for */
} i8042_state = { 0, AUX_DISAB, 0, 0, 0 };

int i8042_has_aux_port = -1;
int i8042_aux_state = 0;

#define I8042_TIMEOUT_SPIN	5000000
#define	I8042_TIME_TO_SOAK	5000000

/*
 * UTILITIES ROUTINES 
 */


unsigned char
Read8042DataByte(waitflg)
int	waitflg;
{
	unsigned char byte;
	register count = 0;
	
	do {
		if(inb(KB_STAT) & KB_OUTBF) {
			byte = inb(KB_IDAT);
			return(byte);  
		}
		count++;
	} while (waitflg && count < I8042_TIMEOUT_SPIN);

#ifdef DEBUG
	if(waitflg)
		cmn_err(CE_NOTE, "Read8042DataByte: timeout!!!");
#endif
	return(0);  
}	
	
time_to_soak()
{
	register int	waitcnt;

	waitcnt = I8042_TIME_TO_SOAK; 
	while ((inb(KB_STAT) & KB_INBF) != 0 && waitcnt-- != 0);
}

int
Write8042DataByte(byte)
unsigned char byte;
{

	register count = 0;
	register unsigned char sts;
	

	do {
		if(((sts = inb(KB_STAT)) & (KB_OUTBF|KB_INBF)) == 0) {
			outb(KB_OUT, byte);
			time_to_soak();
			return(1);
		}
		if(sts&KB_OUTBF)
			inb(KB_IDAT);
		count++;
	} while (count < I8042_TIMEOUT_SPIN);

#ifdef DEBUG
	cmn_err(CE_NOTE, "Write8042DataByte: timeout!!!");
#endif
	return(0);
}	

int
Write8042CommandByte(byte)
unsigned char byte;
{
	register count = 0;
	register unsigned char sts;
	

	do {
		if(((sts = inb(KB_STAT)) & (KB_OUTBF|KB_INBF)) == 0) {
			outb(KB_ICMD, byte);
			time_to_soak();
			return(1);
		}
                if(sts&KB_OUTBF)
                        inb(KB_IDAT);
		count++;
	} while (count < I8042_TIMEOUT_SPIN);

#ifdef DEBUG
	cmn_err(CE_NOTE, "Write8042CommandByte: timeout!!!");
#endif
	return(0);
}	

/*
 * Determine if machine has an auxiliary device port. Return 1 if yes,
 * 0 if no.
 */

int
i8042_aux_port()
{
	int tmp1=0,tmp2=0, oldpri;

	i8042_has_aux_port = 0;
	oldpri = splstr();
	Write8042CommandByte(0xa8);/* enable auxiliary interface */
	Write8042CommandByte(0x20);/* read command byte */
	tmp1 = Read8042DataByte(1);
	if(tmp1 & 0x20){	/* enable did not take */
		splx(oldpri);
		return (i8042_has_aux_port);
	}
	Write8042CommandByte(0xa7);/* disable auxiliary interface */
	Write8042CommandByte(0x20);/* read command byte */
	tmp2 = Read8042DataByte(1);
	if(tmp2 & 0x20){	/* disable successful */
		i8042_has_aux_port = 1;
	}
	splx(oldpri);

	return (i8042_has_aux_port);
}

/*
 * Modify "state" of 8042 so that the next call to release_8042
 * changes the 8042's state appropriately.
 */

void
i8042_program(cmd)
int	cmd;
{
#ifdef DEBUG
	cmn_err(CE_NOTE,"!i8042_program cmd %x",cmd);
#endif
	switch (cmd) {
	case P8042_KBDENAB:	
		i8042_state.s_state &= ~KBD_DISAB;
		break;
	case P8042_KBDDISAB:
		i8042_state.s_state |= KBD_DISAB;
		break;
	case P8042_AUXENAB:
		i8042_state.s_state &= ~AUX_DISAB;
		break;
	case P8042_AUXDISAB:	
		i8042_state.s_state |= AUX_DISAB;
		break;
	default:
		cmn_err(CE_PANIC, "program_8042: illegal command %x", cmd);
		break;
	}
}

/*
 * Acquire the 8042 by changing to splstr, disabling the keyboard and auxiliary
 * devices (if any), and saving any data currently in the 8042 output port.
 */

void
i8042_acquire()
{
	unsigned char cb;

	i8042_state.s_spl = splstr(); 

	cb = inb(KB_STAT)&KB_I8042FLAG; /* save system flag */
	cb |= 0x10|0x20|KB_XLATE; /* disable keyboard and aux */
	Write8042CommandByte(KB_WCB);	/* send new cb to disable interrupts */
	Write8042DataByte(cb);
}

/*
 * Release the 8042.  If data was saved by the acquire, write back the
 * data to the appropriate port, enable the devices interfaces where
 * appropriate and restore the interrupt level.
 */

void
i8042_release()
{
	unsigned char cb; 

	cb = inb(KB_STAT)&KB_I8042FLAG; /* save system flag */

	cb |= KB_EOBFI | KB_XLATE;
	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)) {
		cb |= KB_AUXEOBFI;		/* set aux interrupt in ib */
	}
	else	cb |= 0x20;
	Write8042CommandByte(KB_WCB);	/* send new command byte */
	Write8042DataByte(cb);

	splx(i8042_state.s_spl); 
}

/*
 * Send a command to a device attached to the 8042.  The cmd argument is the
 * command to send.  Whence is the device to send it to.  Bufp is an array of
 * unchars into which any responses are placed, and cnt is the number of bytes
 * expected in the response. Return 1 for success, 0 for failure.
 */

int
i8042_send_cmd(cmd, whence, bufp, cnt)
unchar	cmd,
	whence,
	*bufp,
	cnt;
{
	register unchar	tcnt;
	int	rv = 1, lcnt;

	switch (whence) {
	case P8042_TO_KBD:	/* keyboard */
		break;
	case P8042_TO_AUX:	/* auxiliary */
		Write8042CommandByte(0xd4);
		break;
	default:
#ifdef DEBUG
		cmn_err(CE_NOTE, "send_8042_dev: unknown device");
#endif
		return 0;
	}
	if(inb(KB_STAT) & KB_OUTBF) 
		inb(KB_IDAT);
	Write8042DataByte(cmd);
	for (tcnt = 0; tcnt < cnt; tcnt++) {
		lcnt = I8042_TIMEOUT_SPIN;
		while ((inb(KB_STAT) & KB_OUTBF) == 0 && lcnt--);
		if (lcnt > 0)
			bufp[tcnt] = inb(KB_OUT);
		else {
			rv = 0;
			break;
		}
	}
	return(rv);
}


unsigned short ws_ckbstate = 0, ws_lkbstate = 07;

i8042_reset()
{
	register unsigned char	ledstat;
	unchar	ack;
	int	oldpri = splstr();

	ack = 0;
	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)){
		Write8042CommandByte(0xa7);/* disable auxiliary interface */
		i8042_send_cmd(0xf5, P8042_TO_AUX, &ack, 1);
		}

	if(ws_ckbstate != ws_lkbstate) { 
		/* disable keyboard */
		i8042_send_cmd(0xf5, P8042_TO_KBD,&ack,1);
		i8042_send_cmd(LED_WARN, P8042_TO_KBD,&ack,1);
		ws_lkbstate = ws_ckbstate;
		if (ack == KB_ACK) {
			ledstat = 0;
			if (ws_ckbstate & CAPS_LOCK)
				ledstat |= LED_CAP;
			if (ws_ckbstate & NUM_LOCK)
				ledstat |= LED_NUM;
			if (ws_ckbstate & SCROLL_LOCK)
				ledstat |= LED_SCR;
			i8042_send_cmd(ledstat, P8042_TO_KBD,&ack,1);
		}
		i8042_send_cmd(0xf4, P8042_TO_KBD,&ack,1);
	}
	if (i8042_has_aux_port && !(i8042_state.s_state & AUX_DISAB)) {
#ifdef DEBUG
		if(i8042_aux_state)
			cmn_err(CE_NOTE, "i8042_aux_state is %d",
                                i8042_aux_state);
#endif

		i8042_send_cmd(0xf4, P8042_TO_AUX, &ack, 1);
		Write8042CommandByte(0xa8); /* enable auxiliary interface */
		i8042_aux_state = 0;
	}
	splx(oldpri);
}
