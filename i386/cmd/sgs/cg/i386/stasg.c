/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:i386/stasg.c	1.7.1.9"
/*	stasg.c - machine dependent grunge for back end
 *	i386 CG
 *              Intel iAPX386
 */

# include "mfile2.h"

static void smovregstate();
static void bringtoreg();
static void adrstasg();
static void incby();

extern int zflag;
extern RST regstused;   	/* List of registers used for structure moves */
extern int vol_opnd, special_opnd;
extern int cur_opnd;
#define CLEAN()     {vol_opnd = 0; special_opnd = 0; cur_opnd = 1;}

#define istnode(p)	((p)->in.op==REG && istreg((p)->tn.rval))


    /* Threshold value for unrolling string move into word moves.  The
     * value is computed as
     *              .-                                                   -.
     *              | smov + mov*(sibusy+dibusy) + load*(2-(siset+diset)) |
     * Threshold =  | --------------------------------------------------- |
     *              |                 mov - rep                           |
     * smov is the number of cycles to start a smov == 7
     * mov is the cost to load/store                == 6
     * load is the cost to load an address          == 4 if must use mov,
     *                                              == 2 if can use lea.
     * rep is the cost to "rep smov" a word - smov  == 4
     * [sd]ibusy is 1/0 if/ifnot [sd]i has an active value
     * [sd]iset is 1/0 if/ifnot [sd]i has correct address value as temp
     */
# define THRESHOLD(lop,rop,sibusy,dibusy,siset,diset)                        \
	( ( 1 + 7 + 6*( (sibusy)+(dibusy) ) +                                \
	    (( (lop)!=STAR && (lop)!=VPARAM && (lop)!=TEMP &&                \
		    (lop)!=VAUTO && (lop)!=NAME )             ? 2 : 4 ) *    \
		( 1 - (diset) ) +                                            \
	    (( (rop)!=STAR && (rop)!=VPARAM && (rop)!=TEMP &&                \
		    (rop)!=VAUTO && (rop)!=NAME )             ? 2 : 4 ) *    \
		( 1 - (siset) )                                           )  \
	  / 2 )

void
stasg( l, r, stsize, q )
register NODE *l, *r;
register int stsize;
OPTAB *q;
{
    int sibusy, siset, dibusy, diset;
    int stwords, stchars;

    int vol_flg = 0;
    if ( (l->in.strat & VOLATILE) || (r->in.strat & VOLATILE))
        vol_flg = (VOL_OPND1 | VOL_OPND2);

    smovregstate( &sibusy, &siset, REG_ESI, r );
    smovregstate( &dibusy, &diset, REG_EDI, l );

    stwords = stsize/SZLONG;
    stchars = ( stsize%SZLONG )/SZCHAR;
    if( stwords >=
	    THRESHOLD( l->in.op, r->in.op, sibusy, dibusy, siset, diset ) ) {
	/* Use rep smov for stasg */

	int llreg = optype(l->in.op) == LTYPE ? -1 : regno(l->in.left);
	int rlreg = optype(r->in.op) == LTYPE ? -1 : regno(r->in.left);
	int lreg  = regno(l);
	int rreg  = regno(r);

	    /* special code for stargs so we can still adress off %esp */
	if( l->in.op == PLUS && llreg == REG_ESP && sibusy+dibusy ) 
	    l->in.right->tn.lval += ( sibusy + dibusy ) * ( SZINT/SZCHAR );
	if( r->in.op == PLUS && rlreg == REG_ESP && sibusy+dibusy ) 
	    r->in.right->tn.lval += ( sibusy + dibusy ) * ( SZINT/SZCHAR );

	if ( lreg == REG_ESI ) {
		if ( rreg == REG_EDI ) {
			emit_str("\tpushl\t%esi\n\tpushl\t%edi\n\txchgl\t%esi,%edi\n");
		} else {
			bringtoreg( l, REG_EDI, q, diset, dibusy );
			bringtoreg( r, REG_ESI, q, siset, sibusy );
		}
	} else {
		bringtoreg( r, REG_ESI, q, siset, sibusy );
		bringtoreg( l, REG_EDI, q, diset, dibusy );
	}

	fprintf( outfile,"\tmovl\t$%d,%%ecx\n\trep\n\tsmovl", stwords );
	if( zflag ) {
	    emit_str( "\t\t/ STASG\n");
	}
	putc( '\n',outfile );

	/* Special case for the rep smovl. */
	if (vol_flg) PUTS("/VOL_OPND 0\n");

	if (stchars & 2) {
	    stchars -= 2;
	    fprintf( outfile,"\tmovw\t%d(%%esi),%%cx\n", stchars);
	    if (vol_flg) PUTS("/VOL_OPND 1\n");
	    fprintf( outfile,"\tmovw\t%%cx,%d(%%edi)\n", stchars );
	    if (vol_flg) PUTS("/VOL_OPND 2\n");
	}
	if ( stchars-- ) {
	    fprintf( outfile,"\tmovb\t%d(%%esi),%%cl\n", stchars );
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    fprintf( outfile,"\tmovb\t%%cl,%d(%%edi)\n", stchars );
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	}

	if ( lreg == REG_ESI ) {
		if ( rreg == REG_EDI ) {
			    emit_str( "\tpopl\t%edi\n" );
			    emit_str( "\tpopl\t%esi\n" );
		} else {
			if( sibusy )
			    emit_str( "\tpopl\t%esi\n" );
			if( dibusy )
			    emit_str( "\tpopl\t%edi\n" );
		}
	} else {
		if( dibusy )
		    emit_str( "\tpopl\t%edi\n" );
		if( sibusy )
		    emit_str( "\tpopl\t%esi\n" );
	}

    } else {
	/* Unroll stasg into multiple movl x,%ecx / movl %ecx,y */
	NODE lt, rt;
	char *comment;

	comment = zflag ? "\t\t/ STASG\n" : "\n" ;

	adrstasg( &rt, r, q, 2 );
	adrstasg( &lt, l, q, 3 );

	while( stwords-- ) {
	    expand( &rt, FOREFF, "\tmovl\tA.,%ecx", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    expand( &lt, FOREFF, "\tmovl\t%ecx,A.", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	    incby( &rt, SZLONG/SZCHAR );
	    incby( &lt, SZLONG/SZCHAR );
	}

	if ( stchars & 2 ) {
	    stchars -= 2;
	    expand( &rt, FOREFF, "\tmovw\tA.,%cx", q );
	    emit_str( comment);
            CLEAN(); 
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    expand( &lt, FOREFF, "\tmovw\t%cx,A.", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	    incby( &rt, SZSHORT/SZCHAR );
	    incby( &lt, SZSHORT/SZCHAR );
	}

	if ( stchars-- ) {
	    expand( &rt, FOREFF, "\tmovb\tA.,%cl", q );
	    emit_str( comment);
            CLEAN(); 
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    expand( &lt, FOREFF, "\tmovb\t%cl,A.", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	    incby( &rt, 1 );    /* SZCHAR/SZCHAR */
	    incby( &lt, 1 );
	}
	switch(optype(lt.in.op)) {
	case BITYPE:
		tfree(lt.in.right);
		/* FALLTHRU */
	case UTYPE:
		tfree(lt.in.left);
	}

	switch(optype(rt.in.op)) {
	case BITYPE:
		tfree(rt.in.right);
		/* FALLTHRU */
	case UTYPE:
		tfree(rt.in.left);
	}
    }
}

    /* Determine state of an index register for use in an smov.  Return
     * flags indicating if register reg is set properly for tree p, or
     * failing that, if reg has a live value in it.
     */
static void
smovregstate( used, set, reg, p )
int *used, *set, reg;
register NODE *p;
{
    extern char request[];

    if( istnode( p ) && p->in.op == REG && p->tn.rval == reg ) {
	*set = 1;
	*used = 0;
    } else {
	*set = 0;
	*used = request[reg];
    }
}

    /* Bring address represented by tree p into register reg, anticipatory
     * to doing an smov.  Register reg's state is indicated by whether it
     * is already set, or has a live value in it and therefore is busy.
     */
static void
bringtoreg( p, reg, q, set, busy )
register NODE *p;
int reg, set, busy;
OPTAB *q;
{
NODE t;

    if( !set ) {
	if( busy ) {
	    fprintf( outfile,"\tpushl\t%s\n", rnames[reg] );
	} else
	    regstused |= RS_BIT(reg);
	switch( p->in.op ) {
	case VAUTO:
	case TEMP:
	case VPARAM:
	case NAME:
	case STAR:
	case REG:
	case ICON:
	    if ( p->tn.rval != reg ) {
		expand( p, FOREFF, "\tmovl\tA.,", q );
		fprintf( outfile,"%s\n", rnames[reg] );
	    }
	    break;
	case CSE:	
	    {
		int preg = regno(p);
		if ( preg != reg ) {
			expand( p, FOREFF, "\tmovl\tA.,", q );
			fprintf( outfile,"%s\n", rnames[reg] );
		}
		break;
	    }
	default:
	    t.in.op = STAR;
	    t.in.left = p;
	    expand( &t, FOREFF, "\tleal\tA.,", q );
	    fprintf( outfile,"%s\n", rnames[reg] );
	    break;
	}
    }
}

    /* Return a new tree, starting with the existing NODE at newp, that
     * represents an addressing mode suitable for movl.  This is used
     * for generating unrolled stasgs.
     */
static void
adrstasg( newp, p, q, tempno )
register NODE *newp, *p;
OPTAB *q;
int tempno;
{
    int regnum;

    newp->tn.type = p->in.type;
    switch( p->in.op ) {
    case NAME:
    case STAR:
    case VAUTO:
    case TEMP:
    case VPARAM:    /* pick up the address into a scratch reg */
	if( tempno == 2 )
	    expand( p, FOREFF, "\tmovl\tA.,A2\n", q );
	else if( tempno == 3 )
	    expand( p, FOREFF, "\tmovl\tA.,A3\n", q );
	else
	    expand( p, FOREFF, "\tmovl\tA.,A1\n", q );
	regnum = resc[tempno-1].tn.rval;
	goto starreg;
    case REG:       /* make it into an OREG */
    case CSE:
	regnum = regno(p);
starreg:
	if( regnum == REG_EBP ) {
	    newp->tn.op = VAUTO;
	} else {
	    newp->tn.op = STAR;
	    ( newp = ( newp->in.left = talloc() ) )->in.op = PLUS;
	    ( newp->in.left = talloc() )->in.op = REG;
	    ( newp->in.right = talloc() )->in.op = ICON;
	    newp->in.type = newp->in.left->tn.type = p->in.type;
	    newp->in.left->tn.rval = regnum;
	    newp->in.right->tn.name = 0;
	    newp->in.right->tn.lval = 0;
	}
	break;
    case ICON:
	*newp = *p;
	newp->tn.op = NAME;
	break;
    case UNARY AND:
	*newp = *p;
	newp->in.left = tcopy( p->in.left );
	break;
    default:
	newp->in.op = STAR;
	newp->in.left = tcopy( p );
	break;
    }
}

    /* Increment an address for use in each iteration of the unrolled stasg */
static void
incby( p, inc )
register NODE *p;
register inc;
{

recurse:
    switch( p->in.op ) {
    case VPARAM:
    case VAUTO:
    case NAME:
    case ICON:
	p->tn.lval += inc;
	break;
    case TEMP:
	p->tn.lval += inc * SZCHAR;
	break;
    case UNARY AND:
    case STAR:
	p = p->in.left;
	goto recurse;
    case PLUS:
    case MINUS:
	if( p->in.right->in.op == ICON ) {
	    p->in.right->tn.lval += inc;
	} else {
	    p->in.left->tn.lval += inc;
	}
	break;
    default:
	cerror( "incby: bad node op" );
	/* NOTREACHED */
    }
}

