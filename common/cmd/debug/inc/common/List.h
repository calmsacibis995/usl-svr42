/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	LIST_H
#define LIST_H
#ident	"@(#)debugger:inc/common/List.h	1.2"

// Singly linked queue package

// to loop through list:
// List		*p;
// Item		*i;
//  
//  i = (Item *)p->first();
//  while (i)
//  {
// 	i = (Item*)p->next();
//  }

class ListItem {
	void		*ldata;
	ListItem	*lnext;
	friend class 	List;
};

class List {
	ListItem	*head;
	ListItem	*tail;
	ListItem	*current;
public:
	void		*item()	  { return current->ldata; };
	void		add(void *); // at end
	int		isempty() { return (head == 0); };
	void		*first()  { if ((current=head) == 0) return 0;
					else return current->ldata; };
	void		*last()	  { if (!tail) return 0; else return tail->ldata; }
	void		*next()	  { current=current->lnext;
					return current?current->ldata:0; };
	int		remove(void *);  // remove one entry
	void		clear();  // walks the list and removes all entries
	void		insert(void *); 
			// insert before "current"; if current is null,
			// then add to end

			List()	  { head = 0; tail = 0; current = 0; };
			~List()   { clear(); }
};

#endif // List.h
