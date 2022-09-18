/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/scopes.h	1.3"

#define MAX_DENSE 16

/* hightest positive integer that is a power of 2 */
#define MAX_WEIGHT  0x40000000 / MAX_DENSE

	/* Globals -- put in amigo.h -- PSP */
Boolean PROTO(	build_scopes, (Arena,Block));
int PROTO(	cg_q_in_scope, (Cgq_index, Object));
void PROTO(	order_cg_q, (Arena, Block));
Scope PROTO(	scope_of_cg_q, (cgq_t *));
void PROTO(	bump_scope_usage, (Object, Scope, int));
int PROTO(	scope_usage, (Object, Scope));
Cgq_index PROTO(get_first_scope_index, (Scope, Block *));
Cgq_index PROTO(spill_restore_index, (Scope));
Boolean PROTO(	is_spilled, (Object, Scope));
Scope PROTO(	parent_scope, (Scope));
int PROTO(	spill_cost, (Scope));
Boolean PROTO(	is_live_at_restore_index, (Object, Cgq_index, Scope));

	/* 
	** Macroize following, once scope gets solidified.
	*/
#define SPILL_SAVE_INDEX(scope) (get_first_scope_index(scope,0))
#define SET_SPILL_SAVE_BLOCK(scope, block) ((void)get_first_scope_index(scope, &(block)))
