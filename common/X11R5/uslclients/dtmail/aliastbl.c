/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtmail:aliastbl.c	1.7"
#endif

#define ALIASTBL_C

#include <stdio.h>
#include <string.h>

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/FList.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include "mail.h"

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#define STACK_DEFINE(name, type, size)                    \
	type name##Stack[size];                                \
	type * name;

#define STACK_ALLOCATE(name, type, size)                  \
	if (sizeof(name##Stack)/sizeof(name##Stack[0]) < size) \
		name = (type *)MALLOC(sizeof(type) * size);         \
	else                                                   \
		name = name##Stack;

#define STACK_FREE(name)                                  \
	if (name != name##Stack)                               \
		FREE(name);

AliasTable *		ReadAlias(char * file);
static Token *      Tokenize(char * string);
static void         SortAliasTable(AliasTable * aliasTable);
static int          qstrcmp(const void * s1, const void * s2);
static char *       _GenerateAliasTable(char * name, AliasTable * aliasTable, 
							char recurse);
int         		DeleteAlias(char * name, AliasTable * aliasTable);


/* 
 * ReadAlias
 *
 */

AliasTable *
ReadAlias(char * file)
{
TextBuffer *	textBuffer = ReadFileIntoTextBuffer(file, NULL, NULL);
AliasTable *	aliasTable = NULL;
int				i;
int				j;
char *			p;
char *			q;
Token *			tokens;


	aliasTable = (AliasTable *)AllocateBuffer(sizeof(aliasTable[0]), 10);

	if (textBuffer == NULL) return (aliasTable);	
							/* nothing to do, but need buffer anyway */

	q = NULL;
	for (i = 0; i < LinesInTextBuffer(textBuffer); i++)
	{
		p=GetTextBufferLine(textBuffer, i);
		if (q != NULL)
		{
			q = REALLOC (q, strlen (q) + strlen (p) + 1);
			strcat (q, p);
			FREE (p);
			p = q;
		}
		if (p[(j = strlen (p)-1)] == '\\')
		{
			p[j] = ' ';
			if (i < LinesInTextBuffer(textBuffer)-1)
			{
				q = p;
				continue;
			}
		}
		for (q = p; *q && (*q == ' ' || *q == '	'); q++); /* blank or tab */

		if ((strncmp(q, "alias ", 6)==0 || strncmp(q, "alias	", 6)==0) ||
			(strncmp(q, "group ", 6)==0 || strncmp(q, "group	", 6)==0))
		{
			tokens = Tokenize(q);
			for (j = 2; j < tokens->used - 1; j++)
			{
				if (BufferFilled(aliasTable))
					GrowBuffer((Buffer *)aliasTable, 10);
				aliasTable->p[aliasTable->used].name  = tokens->p[1];
				aliasTable->p[aliasTable->used].addr  = tokens->p[j];
				aliasTable->p[aliasTable->used].inUse = 0;
				aliasTable->used++;
			}
		}
		else
		{
			FREE(p);
		}
		q = NULL;
	}
	FreeTextBuffer(textBuffer, (TextUpdateFunction)0, (caddr_t)0);

	SortAliasTable(aliasTable);

	return(aliasTable);

} /* end of ReadAlias */


/*
 * Tokenize
 *
 */

static Token *
Tokenize(char * string)
{
char *			p = string;
char *			q = NULL;
static Token *	tokens;

	if (tokens == NULL)
		tokens = (Token *)AllocateBuffer(sizeof(tokens[0]), 10);
	
	tokens->used = 0;

	for (q = strtok(string, " ,\t"); q != NULL; q = strtok(NULL, " ,\t"))
	{
		if (BufferFilled(tokens))
			GrowBuffer((Buffer *)tokens, 10);
		tokens->p[tokens->used++] = q;
	}
	tokens->p[tokens->used++] = q;
	
	return (tokens);

} /* end of Tokenize */
/*
 * qstrcmp
 *
 */

static int
qstrcmp(const void * s1, const void * s2)
{
	Alias * p1 = ((Alias *)s1);
	Alias * p2 = ((Alias *)s2);

	return (strcmp(p1->name, p2->name));

} /* end of qstrcmp */
/*
 * SortAliasTable
 *
 */

static void
SortAliasTable(AliasTable * aliasTable)
{

	qsort((void *)aliasTable->p, 
		(size_t)aliasTable->used, sizeof(aliasTable->p[0]), qstrcmp);

} /* end of SortAliasTable */


/*
 * GenerateAliasTable
 *
 */

extern char *
GenerateAliasTable(char * name, AliasTable * aliasTable, char recurse)
{
int		i;

	for (i = 0; i < aliasTable->used; i++)
		aliasTable->p[i].inUse = FALSE;

	return(_GenerateAliasTable(name, aliasTable, recurse));

} /* end of GenerateAliasTable */
/*
 * _GenerateAliasTable
 *
 */

static char *
_GenerateAliasTable(char * name, AliasTable * aliasTable, char recurse)
{

int		i;
int		j;
int		k;
char *	t;
int		len = strlen(name) + 1;
int		found     = FALSE;
char *	addr      = MALLOC(1);
Token *	tokens;

	STACK_DEFINE(aliasCopy, char, 100);

	STACK_ALLOCATE(aliasCopy, char, len);

	(void)strcpy(aliasCopy, name);
	tokens = Tokenize(aliasCopy);

	*addr = 0;
	len = 0;

	if (tokens)
	{
		tokens = (Token *)CopyBuffer((Buffer *)tokens);
		for (j = 0; j < tokens->used - 1; j++)
		{
			name = tokens->p[j];
			found = FALSE;

			for (i = 0; i < aliasTable->used; i++)
				if (strcmp(aliasTable->p[i].name, name) == 0)
				{
					found = TRUE;
					if (!recurse)
					{
						FREE (addr);
						addr = STRDUP (aliasTable->p[i].addr);
						/* find other addresses associated with this name */
						len = strlen (addr);
						for (k = i + 1; k < aliasTable->used; k++)
						{
							if (strcmp(aliasTable->p[k].name,name) != 0) break; 
							len += (strlen(aliasTable->p[k].addr) + 2);
							addr = REALLOC(addr, len);
							strcat(addr, " ");
							strcat(addr, aliasTable->p[k].addr);
						}
						break;		/* done searching */
					}
					else if (aliasTable->p[i].inUse == FALSE)
					{
						aliasTable->p[i].inUse = TRUE;
						t = _GenerateAliasTable
							(aliasTable->p[i].addr, aliasTable, recurse);
						if (*t)
						{
							len += (strlen(t) + 2);
							addr = REALLOC(addr, len);
							if (*addr) strcat(addr, " ");
							strcat(addr, t);
							FREE(t);
						}
					}
				}
			if (!found)
			{
				len += (strlen(name) + 2);
				addr = REALLOC(addr, len);
				if (*addr)
					strcat(addr, " ");
				strcat(addr, name);
			}
		}
		FreeBuffer((Buffer *)tokens);
	}

	STACK_FREE(aliasCopy);

	return (addr);

} /* end of GenerateAliasTable */
/*
 * ReplaceAlias
 *
 */

int
ReplaceAlias(char * name, char * addresses, AliasTable * aliasTable)
{
int		numDeleted;

	/***  fprintf (stderr, "ReplaceAlias: name <%s>, addr <%s>\n",name,addresses); /***/
	numDeleted = DeleteAlias(name, aliasTable);

	/***  fprintf (stderr, "ReplaceAlias: numDeleted %d\n",numDeleted); /***/
	if (addresses)
	{
		char *  aliasCopy   = strcpy(MALLOC(strlen(name) + 1), name);
		char *  addressCopy = strcpy(MALLOC(strlen(addresses) + 1), addresses);
		Token * tokens      = Tokenize(addressCopy);
		int     j;

		/***  fprintf (stderr, "ReplaceAlias: tokens->used %d, name <%s>, addr <%s>\n",tokens->used,name,addresses); /***/
		if (tokens)
			for (j = 0; j < tokens->used - 1; j++)
			{
				/***  fprintf (stderr, "ReplaceAlias: j %d, aliasTable->used %d, aliasTable <%#x>\n",j,aliasTable->used,aliasTable); /***/
				if (BufferFilled(aliasTable))
					GrowBuffer((Buffer *)aliasTable, 10);
				aliasTable->p[aliasTable->used].name  = aliasCopy;
				aliasTable->p[aliasTable->used].addr  = tokens->p[j];
				aliasTable->p[aliasTable->used].inUse = 0;
				aliasTable->used++;
			}
	}

	SortAliasTable(aliasTable);

	return (numDeleted);

} /* end of ReplaceAlias */
/*
 * DeleteAlias
 *
 */

int
DeleteAlias(char * name, AliasTable * aliasTable)
{
int		i;
int		begin = -1;
int		end;
int		numDeleted = 0;

	for (i = 0; i < aliasTable->used; i++)
		if (strcmp(name, aliasTable->p[i].name) == 0)
			if (begin != -1)
				end = i;
			else
				end = begin = i;
		else
			if (begin != -1)
				break;

	if (begin != -1)
	{
		numDeleted = end - begin + 1;
		if (end == aliasTable->used - 1)
		{
			aliasTable->used = begin;
		}
		else
		{
			(void)memcpy(&aliasTable->p[begin], 
					&aliasTable->p[end + 1], 
					sizeof(aliasTable->p[0]) * (aliasTable->used - end - 1));
			aliasTable->used -= numDeleted;
		}
	}

	return (numDeleted);

} /* end of DeleteAlias */
