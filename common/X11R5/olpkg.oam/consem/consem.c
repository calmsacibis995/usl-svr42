/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olpkg.oam:consem/consem.c	1.2"
#endif

/* 
 * X Window Console Emulator
 * pushable streams module
 */
#include "sys/param.h"
#include "sys/signal.h"
#include "sys/types.h"
#include "sys/immu.h"
#include "sys/systm.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/fs/s5dir.h"
#include "sys/user.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/conf.h"
#include "sys/tty.h"
#ifndef SVR4_0
#include "sys/ttold.h"
#include "sys/cred.h"
#endif
#include "sys/termio.h"
#include "sys/jioctl.h"
#include "sys/errno.h"
#include "sys/cmn_err.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "sys/vt.h"
#include "sys/consem.h"

#define CSEMDBG 1
#ifdef CSEMDBG
#define DEBUG1(a)	 if (csem_debug) cmn_err(CE_CONT,a) 
#define DEBUG2(a)	 if (csem_debug > 1) cmn_err(CE_CONT,a) 
#define DEBUG4(a)	 if (csem_debug > 3) cmn_err(CE_CONT,a) 
#endif

int csem_debug = 0;
extern int csem_cnt;
extern int csem_tab_siz;
extern struct csem csem[];
extern struct csem_esc_t csem_tab[];

int consemopen(), consemclose(), consemin(), consemout();
extern nulldev();
struct module_info consemmiinfo = { 
/*       ID       Name   minpac maxpac hiwat lowat */
	0x00ce, "consem",  0,   512, 300,  100 };

struct module_info consemmoinfo = { 
	0x00ce, "consem",  0,   512, 300,  200 };

struct qinit consemrinit = {
	consemin, NULL, consemopen, consemclose, NULL, &consemmiinfo, NULL};

struct qinit consemwinit = {
	consemout, NULL, consemopen, consemclose, NULL, &consemmoinfo, NULL};

struct streamtab cseminfo = { 
	&consemrinit, &consemwinit };

csem_tout()
{
	register struct csem *csp;	/* Pointer to a conseme structure */
	int flag=0;

	DEBUG1 (("consem timeout\n"));
	for ( csp = csem; csp <= &csem[csem_cnt-1]; csp++){
		if ( csp->state&WAITING){
			csp->iocp->ioc_count=0;
			csp->iocp->ioc_error=ETIME;
			csp->c_mblk->b_datap->db_type = M_IOCNAK;
			csp->state &= ~WAITING;
			qreply(csp->c_q, csp->c_mblk);
			flag++;
			break;
		}
	}
	if (flag==0)
		DEBUG1 (("consem: NOTHING to timeout\n")); 
	return;
}

consemopen(q, dev, oflag, sflag)
register queue_t *q;
int	dev, oflag, sflag;
{
	register mblk_t *mp;
	register struct csem *csp;	/* Pointer to a conseme structure */

	DEBUG1 (("consemopen called\n"));

	if (sflag != MODOPEN) {
		u.u_error = EINVAL;
		return (OPENFAIL);
	}
	if (q->q_ptr != NULL) 	/* already attached */
		return(1);
	
	for ( csp = csem; csp->state&CS_OPEN; csp++)
		if ( csp >= &csem[csem_cnt-1]) {
			DEBUG1(("No consem structures.\n"));
			u.u_error = ENODEV;
			return( OPENFAIL);
		}
	csp->state = CS_OPEN;
	q->q_ptr 	= (caddr_t)csp;
	WR(q)->q_ptr 	= (caddr_t)csp;
	return;

}

consemclose(q)
register queue_t *q;
{
	struct csem *csp = (struct csem *)q->q_ptr;

	DEBUG1 (("consemclose\n")); 
	if ( csp->state&WAITING){
		untimeout(csp->to_id);
		freemsg(csp->c_mblk);
	}
	csp->state = 0;
	q->q_ptr = NULL;
	return;
}


/*
 *  put procedure for input from driver end of stream (read queue)
 */
consemin(q, mp)
register queue_t *q;
register mblk_t *mp;
{

	struct copyreq *send_buf_p;
	register struct csem *csp;
	mblk_t *tmp;
	struct iocblk *iocp;
	int i;
	int outsiz;
	char *cptr, pass_fail;

	DEBUG4 (("consemin:\n"));
	csp = (struct csem *)q->q_ptr;

	if (mp->b_datap->db_type != M_DATA) {
		DEBUG1(("consemin: Non DATA default\n"));
		putnext(q, mp);
		return;
	}
	if ( csp->state&WAITING){
		untimeout(csp->to_id);
		csp->state &= ~WAITING;
		if ((mp->b_rptr[0] != 033)||(mp->b_rptr[1] != '@')||(mp->b_rptr[2] != '3')) {
			if(csem_debug>3)printf ("Not xterm response %x %x %x %x \n",
		mp->b_rptr[0], mp->b_rptr[1], mp->b_rptr[2],mp->b_rptr[3]);
			goto ce_nxt;
		}
/*
		cptr=(char *)mp->b_rptr; for (i=0; i<40; i++) printf ("%d ", *cptr++); printf("\n");
*/
		if ( csem_tab[csp->indx].type == CSEM_R){
			DEBUG1 ((" consemin: Sending rval response.\n"));
			csp->iocp->ioc_count=0;
			if( (char)*(mp->b_rptr+4)){	/* SC non-zero */
				csp->iocp->ioc_rval = -1;
				csp->c_mblk->b_datap->db_type = M_IOCNAK;
/* joeh */			csp->iocp->ioc_error=EINVAL;
			}
			else{
				bcopy (mp->b_rptr+5, &csp->iocp->ioc_rval,4);
				csp->c_mblk->b_datap->db_type = M_IOCACK;
				csp->iocp->ioc_error=0;
			}
			freemsg(mp);
			putnext(q, csp->c_mblk);
			return;
		}
		/* SCEM_O  */
		else if ( csem_tab[csp->indx].type == CSEM_O){
			if( (char)*(mp->b_rptr+4)){	/* SC non-zero */
				csp->c_mblk->b_datap->db_type = M_IOCNAK;
/* joeh */			csp->iocp->ioc_error=EINVAL;
				csp->iocp->ioc_rval = -1;
				freemsg(mp);
				qreply( csp->c_q, csp->c_mblk);
				return;
			}
			DEBUG1 ((" consemin: Queing COPYOUT response.\n"));
			outsiz=csem_tab[csp->indx].b_out;
			if(csem_debug)printf ("index= %d, outsiz= %d, ioc_count= %d\n", csp->indx, outsiz, csp->iocp->ioc_count);
			if ( csp->iocp->ioc_count == TRANSPARENT) {
				if ((tmp = allocb(outsiz, BPRI_MED)) == NULL)  {
					csp->c_mblk->b_datap->db_type = M_IOCNAK;
					csp->iocp->ioc_error = EAGAIN;
					DEBUG1(("EAGAIN case 1\n"));
					csp->iocp->ioc_rval = -1;
					freemsg(mp);
					qreply( csp->c_q, csp->c_mblk);
					return;
				}
				send_buf_p=(struct copyreq *)csp->c_mblk->b_rptr;
				send_buf_p->cq_addr = (caddr_t)(*(long *)(csp->c_mblk->b_cont->b_rptr));
				freemsg( csp->c_mblk->b_cont);
				csp->c_mblk->b_cont = tmp;
				bcopy( mp->b_rptr+5, tmp->b_rptr, outsiz);
				tmp->b_wptr += outsiz;
				send_buf_p->cq_private = NULL;
				send_buf_p->cq_flag = 0;
				send_buf_p->cq_size=outsiz;
				if(csem_debug)printf("COPYOUT outsize= %d\n", outsiz);
				csp->c_mblk->b_datap->db_type = M_COPYOUT;
				freemsg(mp);
				qreply(csp->c_q, csp->c_mblk);
			}
			else { /* This can't happen */
				csp->c_mblk->b_datap->db_type = M_IOCNAK;
				csp->iocp->ioc_count = 0;
				csp->iocp->ioc_error = EAGAIN;
				DEBUG1(("EAGAIN case 2\n"));
				csp->iocp->ioc_rval = -1;
				freemsg(mp);
				qreply(csp->c_q, csp->c_mblk);
			}
			return;
		}
		/* CSEM_B */
		else if ( csem_tab[csp->indx].type == CSEM_B){
			pass_fail=(char)*(mp->b_rptr+4);
			outsiz=csem_tab[csp->indx].b_out;
			if(csem_debug) printf ("index= %d, outsiz= %d\n", csp->indx, outsiz);
			if ((tmp = allocb(outsiz, BPRI_MED)) == NULL)  {
				DEBUG1(("CSEM_B: 1st buffer alloc failed.\n"));
				putnext(q, mp);
				return;
			}
			if( pass_fail ){	/* SC non-zero */
				DEBUG1(("CSEM_B: return code failed\n"));
				iocp = (struct iocblk *)tmp->b_rptr;
#ifdef SVR4_0
				if ((iocp->ioc_cr = (cred_t *) allocb(sizeof(cred_t), BPRI_MED)) == NULL) {
					DEBUG1(("CSEM_B: 2nd buffer alloc failed.\n"));
					putnext(q, mp);
					return;
				}
#endif
				iocp->ioc_cmd= csp->ioc_cmd;
				iocp->ioc_uid= csp->ioctl_uid;
				iocp->ioc_gid= csp->ioctl_gid;
				iocp->ioc_id= csp->ioc_id;
				iocp->ioc_rval = -1;
				iocp->ioc_error=EINVAL;
				tmp->b_datap->db_type = M_IOCNAK;
				freemsg(mp);
				putnext(q, tmp);
				return;
			}
			DEBUG1(("consemin:CSEM_B: Queing COPYOUT response.\n"));
			send_buf_p=(struct copyreq *)tmp->b_rptr;
			send_buf_p->cq_addr = (caddr_t)(*(long *)(tmp->b_cont->b_rptr));
			bcopy( mp->b_rptr+5, tmp->b_cont->b_rptr, outsiz);
			tmp->b_cont->b_wptr += outsiz;
			send_buf_p->cq_cmd= csp->ioc_cmd;
#ifdef SVR4_0
			if ((send_buf_p->cq_cr = (cred_t *) allocb(sizeof(cred_t), BPRI_MED)) == NULL) {
				DEBUG1(("CSEM_B: 3rd buffer alloc failed.\n"));
				putnext(q, mp);
				return;
			}
#endif
			send_buf_p->cq_uid= csp->ioctl_uid;
			send_buf_p->cq_gid= csp->ioctl_gid;
			send_buf_p->cq_id= csp->ioc_id;
			send_buf_p->cq_private = NULL;
			send_buf_p->cq_flag = 0;
			send_buf_p->cq_size=outsiz;
			if(csem_debug)printf("COPYOUT outsize= %d\n", outsiz);
			tmp->b_datap->db_type = M_COPYOUT;
			csp->state |= CO_REQ;
			freemsg(mp);
			freemsg(csp->c_mblk);
			putnext(q, tmp);
			return;
		}
		else if ( (csem_tab[csp->indx].type == CSEM_I) ||
			  ( csem_tab[csp->indx].type == CSEM_N) ){
			/* joeh is buffer big enough here? */
			DEBUG1 (("consemin: ACKING set or null ioctl.\n"));
			pass_fail=(char)*(mp->b_rptr+4);
			iocp = (struct iocblk *)mp->b_rptr;
#ifdef SVR4_0
			if ((iocp->ioc_cr = (cred_t *) allocb(sizeof(cred_t), BPRI_MED)) == NULL) {
				iocp->ioc_rval = -1;
				mp->b_datap->db_type = M_IOCNAK;
	  			iocp->ioc_error=EINVAL;
			}
#endif
			iocp->ioc_cmd= csp->ioc_cmd;
			iocp->ioc_uid= csp->ioctl_uid;
			iocp->ioc_gid= csp->ioctl_gid;
			iocp->ioc_id= csp->ioc_id;
			if( pass_fail ){	/* SC non-zero */
				iocp->ioc_rval = -1;
				mp->b_datap->db_type = M_IOCNAK;
/* joeh */			iocp->ioc_error=EINVAL;
			}
			else{
				iocp->ioc_rval = 0;
				mp->b_datap->db_type = M_IOCACK;
				iocp->ioc_error=0;
			}
			iocp->ioc_count=0;
			freemsg(csp->c_mblk);
			putnext(q, mp);
			return;
		}
		else
			DEBUG1 (("consemin: ERROR.\n"));
	}

ce_nxt:	putnext(q, mp);
	return;
}

/*
 * output queue put procedure: 
 */
consemout(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct csem *csp;
	struct iocblk *iocp;
	mblk_t *tmp;
	int	insiz, tone, n;
	unsigned char *j;
	struct copyresp *resp;
	int 	cmd;

	DEBUG4(("consemout:\n"));
	csp = (struct csem *)q->q_ptr;
	iocp = (struct iocblk *)mp->b_rptr;
	cmd  = iocp->ioc_cmd;

	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		DEBUG1(("consemout: M_IOCTL\n"));
#ifdef CSEMDBG
		csemprt (cmd);
#endif
#ifdef USL_OLD_MODESWITCH
       		if ((cmd & 0xffffff00) == USL_OLD_MODESWITCH)
            	     cmd = (cmd & ~IOCTYPE) | MODESWITCH;
#endif
		switch (cmd) {
/*		case PIO_KEYMAP:	Disabled	*/
		case PIO_SCRNMAP:
			if (iocp->ioc_uid){
				iocp->ioc_count=0;
				iocp->ioc_error=EACCES;
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				return;
			}	/* else fall through */
		case KDDISPTYPE:
		case KDGKBENT:
		case KDSKBENT:
		case KDGKBMODE:
		case KDSKBMODE:
		case GIO_ATTR:
		case GIO_COLOR:
/*		case GIO_KEYMAP:	Disabled	*/
		case GIO_STRMAP:
		case PIO_STRMAP:
		case GIO_SCRNMAP:
		case GETFKEY:
		case SETFKEY:
		case SW_B40x25:
		case SW_C40x25:
		case SW_B80x25:
		case SW_C80x25:
		case SW_EGAMONO80x25:
		case SW_ENHB40x25:
		case SW_ENHC40x25:
		case SW_ENHB80x25:
		case SW_ENHC80x25:
		case SW_ENHB80x43:
		case SW_ENHC80x43:
		case SW_MCAMODE:
		case O_SW_B40x25:
		case O_SW_C40x25:
		case O_SW_B80x25:
		case O_SW_C80x25:
		case O_SW_EGAMONO80x25:
		case O_SW_ENHB40x25:
		case O_SW_ENHC40x25:
		case O_SW_ENHB80x25:
		case O_SW_ENHC80x25:
		case O_SW_ENHB80x43:
		case O_SW_ENHC80x43:
		case O_SW_MCAMODE:
		case CONS_CURRENT:
		case CONS_GET:
		case TIOCVTNAME:
			consemioc(q, mp);
			return;

		case KDGETMODE:
			DEBUG1(("KDGETMODE case\n"));
			iocp->ioc_count=0;
			iocp->ioc_error=0;
			iocp->ioc_rval=KD_TEXT;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return;

		case KDSETMODE:
		case KDSBORDER:
		case CGA_GET:
		case EGA_GET:
		case PGA_GET:
		case MCA_GET:
		case TIOCKBOF:
		case KBENABLED:
			iocp->ioc_count=0;
			iocp->ioc_error=EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
	
		case KDMKTONE:
			DEBUG1(("KDMKTONE case\n"));
			tone = *(int *)mp->b_cont->b_rptr;
			csem_tone((tone & 0xffff), ((tone >> 16) & 0xffff));
			iocp->ioc_count=0;
			iocp->ioc_error=0;
			iocp->ioc_rval=0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return;
	
		default:
			DEBUG1(("consem: default ioctl case\n"));
			putnext(q, mp);
			return;
		}
	case M_IOCDATA:
		resp = (struct copyresp *)mp->b_rptr;
		if ( resp->cp_rval)  {
			freemsg( mp);		/* Just return on failure */
			return;
		}
		switch ( resp->cp_cmd) {
		case KDDISPTYPE: /* CSEM_O or CSEM_R Pocsessing */
		case KDGKBMODE:
		case GIO_KEYMAP:
		case GIO_STRMAP:
		case GIO_SCRNMAP:
		case TIOCVTNAME:
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply( q, mp);
			return;

		case GETFKEY: 		/* CSEM_B Processing */
		case KDGKBENT:
			if ( csp->state&CO_REQ){
				iocp->ioc_error = 0;
				iocp->ioc_count = 0;
				iocp->ioc_rval = 0;
				mp->b_datap->db_type = M_IOCACK;
				csp->state &= ~CO_REQ;
				qreply( q, mp);
				return;
			}
			/* else: fall through and do COPYIN */

		case KDSKBENT:		/* CSEM_I (COPYIN) Pocsessing */
		case KDSKBMODE: 
		case PIO_KEYMAP:
		case PIO_STRMAP:
		case PIO_SCRNMAP:
		case SETFKEY:
			insiz=mp->b_cont->b_wptr - mp->b_cont->b_rptr;
			if(csem_debug)printf("COPYIN insize= %d\n", insiz);
			if (insiz != csem_tab[csp->indx].b_in)
				if(csem_debug)printf("copyin siz err, sizin= %d, tab=%d, ioctl= %s\n", insiz, csem_tab[csp->indx].b_in, csem_tab[csp->indx]. name);
			if ((tmp=allocb(insiz+4, BPRI_HI)) == NULL ){
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				DEBUG1(("EAGAIN case 3\n"));
				qreply(q, mp);
				return;
			}
			tmp->b_rptr[0] = 033;	/* Escape */
			tmp->b_rptr[1] = '@';
			tmp->b_rptr[2] = '2';
			tmp->b_rptr[3] = csem_tab[csp->indx].esc_at;
			bcopy(mp->b_cont->b_rptr, tmp->b_rptr+4, insiz);
			tmp->b_wptr= tmp->b_rptr + 4 +insiz;
			tmp->b_datap->db_type = M_DATA;
			csp->c_q = q;
			csp->c_mblk=mp;
			csp->ioc_cmd = resp->cp_cmd;
			csp->ioctl_uid = resp->cp_uid;
			csp->ioctl_gid = resp->cp_gid;
			csp->ioc_id = resp->cp_id;
			csp->state |= WAITING;
			csp->to_id=timeout(csem_tout , 0, 20*HZ);
			putnext(q, tmp);
			return;

		default:
			DEBUG1(("default consem: IOC_DATA case\n"));
			putnext(q, mp);
			return;
		}
	}

	putnext(q, mp);
	return;
}

consemioc(q, mp)
mblk_t *mp;
queue_t *q;
{
	register struct csem *csp;
	struct iocblk *iocp;
	mblk_t *tmp;
	int i;

	csp = (struct csem *)q->q_ptr;
	iocp = (struct iocblk *)mp->b_rptr;

	/* Message must be of type M_IOCTL for this routine to be called.  */
	if(csem_debug)printf("consemioc:iocp->ioc_cmd = %x\n", iocp->ioc_cmd);

	for (i=0; i < csem_tab_siz; i++){
		if (csem_tab[i].ioctl == iocp->ioc_cmd){
			if(csem_debug)printf("consemioc: esc match, i=%d\n", i);
			if (csem_tab[i].type & CSEM_I){
				if ( iocp->ioc_count == TRANSPARENT) {
					register struct copyreq *get_buf_p;
					get_buf_p=(struct copyreq *)mp->b_rptr;
					get_buf_p->cq_private = NULL;
					get_buf_p->cq_flag = 0;
					get_buf_p->cq_size = csem_tab[i].b_in;
					get_buf_p->cq_addr = (caddr_t) (*(long*)(mp->b_cont->b_rptr));
					freeb(mp->b_cont);
					mp->b_cont=NULL;
					DEBUG1 ((" consemioc: Queing COPYIN.\n"));
					mp->b_datap->db_type = M_COPYIN;
					csp->indx=i;
					csp->iocp=iocp;
					qreply( q, mp);
					return;
				}
				else{
					goto ce_err;
				}
			}
			else{	/* Not CSEM_I */
				if ((tmp=allocb(sizeof(struct csem_esc_t), BPRI_HI)) == NULL) {
					mp->b_datap->db_type = M_IOCNAK;
					iocp->ioc_error = EAGAIN;
					DEBUG1(("EAGAIN case 4\n"));
					qreply(q, mp);
					return;
				}
				tmp->b_rptr[0] = 033;	/* Escape octal code */
				tmp->b_rptr[1] = '@';
				tmp->b_rptr[2] = '2';
				tmp->b_rptr[3] = csem_tab[i].esc_at;
				tmp->b_wptr= tmp->b_rptr + 4;
				tmp->b_datap->db_type = M_DATA; /*change IOCTL to DATA*/
				csp->c_q = q;
				csp->c_mblk=mp;
				csp->iocp=iocp;
				csp->indx=i;
				csp->ioc_cmd = iocp->ioc_cmd;
				csp->ioctl_uid = iocp->ioc_uid;
				csp->ioctl_gid = iocp->ioc_gid;
				csp->ioc_id = iocp->ioc_id;
				csp->state |= WAITING;
				csp->to_id=timeout(csem_tout , 0, 20*HZ);
				putnext(q, tmp);
				return;
			}
		}
	}
	/* If we got here we could find ioc_cmd in table of esc codes */
	DEBUG1 (("consemioc: esc match errror\n"));
ce_err:	iocp->ioc_count=0;
	iocp->ioc_error=EINVAL;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
	return;
}

#ifdef CSEMDBG
struct csemv{
	int cmd;
	char *str;
} csema[]={
TCGETA,"TCGETA",
TCSETA,"TCSETA",
TCSETAW,"TCSETAW",
TCSETAF,"TCSETAF",
TCSBRK,"TCSBRK",
TCXONC,"TCXONC",
TCFLSH,"TCFLSH",
TIOCSETP,"TIOCSETP",
IOCTYPE,"IOCTYPE",
TIOCSWINSZ,"TIOCSWINSZ",
TIOCGWINSZ,"TIOCGWINSZ",
JWINSIZE,"JWINSIZE",
KDDISPTYPE,"KDDISPTYPE",
KDGKBENT,"KDGKBENT",
KDSKBENT,"KDSKBENT",
KDGKBMODE,"KDGKBMODE",
KDSKBMODE,"KDSKBMODE",
KDSBORDER,"KDSBORDER",
KDGETMODE,"KDGETMODE",
KDSETMODE,"KDSETMODE",
KDMKTONE,"KDMKTONE",
GIO_ATTR,"GIO_ATTR",
GIO_COLOR,"GIO_COLOR",
GIO_KEYMAP, "GIO_KEYMAP",
PIO_KEYMAP, "PIO_KEYMAP",
GIO_STRMAP, "GIO_STRMAP",
PIO_STRMAP, "PIO_STRMAP",
GIO_SCRNMAP, "GIO_SCRNMAP",
PIO_SCRNMAP, "PIO_SCRNMAP",
GETFKEY, "GETFKEY",
SETFKEY, "SETFKEY",
SW_B40x25,"SW_B40x25",
SW_C40x25, "SW_C40x25",
SW_B80x25, "SW_B80x25",
SW_C80x25, "SW_C80x25",
SW_EGAMONO80x25, "SW_EGAMONO80x25",
SW_ENHB40x25, "SW_ENHB40x25",
SW_ENHC40x25, "SW_ENHC40x25",
SW_ENHB80x25, "SW_ENHB80x25",
SW_ENHC80x25, "SW_ENHC80x25",
SW_ENHB80x43, "SW_ENHB80x43",
SW_ENHC80x43, "SW_ENHC80x43",
SW_MCAMODE, "SW_MCAMODE",
CONS_CURRENT,"CONS_CURRENT",
CONS_GET,"CONS_GET",
CGA_GET,"CGA_GET",
EGA_GET,"EGA_GET",
PGA_GET,"PGA_GET",
MCA_GET,"MCA_GET",
KBENABLED,"KBENABLED",
TIOCKBON,"TIOCKBON",
TIOCKBOF,"TIOCKBOF",
TIOCVTNAME,"TIOCVTNAME"
};
csemprt(cmd)
int cmd;
{
	int i, csem_tsiz;

	csem_tsiz = sizeof(csema)/sizeof(csema[0]);
	if (csem_debug > 1){
		printf ("consem: ioctl= %x, ", cmd);
		for (i=0; i<csem_tsiz; i++)
			if (csema[i].cmd == cmd)
				printf ("%s", csema[i].str);
		printf("\n");
	}
}
#endif

int csem_toneon;
/*
 * Sound generator.  This plays a tone at frequency freq
 * for length milliseconds.
 */
csem_tone(freq, length)
unsigned short	freq;
unsigned short	length;
{
	unsigned char	status;
	register int	linhz;
	int		csem_toneoff(), oldpri;

	if (freq == 0)
		return;
	linhz = (int) (((long) length * HZ) / 1000L);
	if (linhz == 0)
		return;
	oldpri = spltty();
	while (csem_toneon)
		sleep(&csem_toneon, TTOPRI);
	splx(oldpri);
	csem_toneon = 1; 
	/*
 	* set up timer mode and load initial value
 	*/
	outb(TIMERCR, T_CTLWORD);
	outb(TIMER2, freq & 0xff);
	outb(TIMER2, (freq>>8) & 0xff);
	/* 
 	* turn tone generator on
 	*/
	status = inb(TONE_CTL);
	status |= TONE_ON;
	outb(TONE_CTL, status);
	timeout(csem_toneoff, 0, linhz);
}


/*
 * Turn the sound generation off.
 */
csem_toneoff()
{
	unsigned char status;

	status = inb(TONE_CTL);
	status &= ~TONE_ON;
	outb(TONE_CTL, status);
	csem_toneon = 0;
	wakeup(&csem_toneon); 
}
