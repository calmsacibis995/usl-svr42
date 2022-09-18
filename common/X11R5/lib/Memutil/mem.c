/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)memutil:mem.c	1.2"

#include <stdio.h>

typedef enum 
   { 
   NotFound, FoundExactMatch, 
   FoundGreaterLine, FoundLesserLine, FoundGreaterFile
   }
   MatchStatus;

#define SameFile      0
#define LastFileEntry 1

typedef struct _Reference
   {
   char *               file;
   int                  line;
   int                  used;
   int                  call;
   int                  flag;
   struct _Reference  * next;
   } Reference;

typedef struct _MemoryMap
   {
   Reference *          ref;
   char *               addr;
   unsigned int         size;
   char                 type;
   char                 used;
   struct _MemoryMap *  next;
   } MemoryMap;
/*
 * the format is:
 * 80ed358 (M)  4108             at   339 in Event.c
 * 01234567890123456789012345678901234567890123456789
 */

#define TOKENIZE() buffer[7] = '\0'
#define ADDR     (&buffer[0])
#define TYPE     (buffer[9])
#define SIZE     (atoi(&buffer[11]))
#define LINE     (atoi(&buffer[33]))
#define FILE     (&buffer[42])
#define FLAG     (buffer[0])

#define TRUE      1
#define FALSE     0

#define ALLOCATION(p)   (p-> type != 'F')
#define DEALLOCATION(p) (p-> type == 'F')
#define REALLOCATION(p) (p-> type == 'R')

#define CALL_LIMIT      100
#define SMALL_LIMIT      10
#define USE_LIMIT      2500

extern char * malloc();
extern char * strdup();

static char *      FlagOutput();
static void        Match();
static Reference * AddReference();

static int    match = 1;

static char * space = "        ";

static char   buffer[4096];

/*
 * FlagOutput
 *
 */

static char * FlagOutput(fld, calls, alloced, count, inuse)
char * fld;
int calls, alloced, count, inuse;
{
   static char b[3];

   if (strcmp(fld, "subtotal") == 0 || strcmp(fld, "total") == 0)
      b[0] = b[1] = b[2] = ' ';
   else
      {
      b[0] = (calls > CALL_LIMIT)  ? 'C' : ' ';
      b[1] = (inuse < SMALL_LIMIT) ? 'T' : ' ';
      b[2] = (inuse > USE_LIMIT)   ? 'S' : ' ';
      }

   return (b);

} /* end of FlagOutput */
/*
 * main
 *
 */

main(argc, argv)
unsigned int argc;
char * argv[];
{
Reference *  ref_start  = NULL;
Reference *  ref_ptr    = NULL;
MemoryMap *  map_start  = NULL;
MemoryMap *  map_ptr    = NULL;
MemoryMap ** map_next   = &map_start;
int          subtotal;
int          subtotal2;
int          filetotal  = 0;
int          filetotal2 = 0;
int          total      = 0;
int          total2     = 0;

if (argc == 2 && strcmp(argv[1], "-n") == 0)
   match = 0;

ref_start = (Reference *) malloc(sizeof(Reference));
ref_start-> file = "  main";
ref_start-> line = 0;
ref_start-> used = 0;
ref_start-> call = 0;
ref_start-> next = NULL;

/*
 * read file into MemoryMap/Reference structures
 */

(void)fprintf(stderr, "r");
while (gets(buffer) != NULL)
   {
   TOKENIZE();
   if ('0' <= FLAG && FLAG <= '9')
      {
      map_ptr = (MemoryMap *) malloc(sizeof(MemoryMap));
      if (map_ptr == NULL)
         (void)fprintf(stderr,"out of memory at %d\n", __LINE__);
      map_ptr-> ref  = AddReference(ref_start, LINE, FILE, TRUE);
      map_ptr-> addr = strdup(ADDR);
      map_ptr-> size = SIZE;
      map_ptr-> type = TYPE;
      map_ptr-> used = TRUE;
      map_ptr-> next = NULL;
      *map_next = map_ptr;
      map_next = &map_ptr-> next;
      }
   }

/*
 * match up the frees and mallocs
 */

(void)fprintf(stderr, "m");
if (match)
   {
   for (map_ptr = map_start; map_ptr != NULL; map_ptr = map_ptr-> next)
      if (ALLOCATION(map_ptr))
         Match(map_ptr);
   }

(void)fprintf(stderr, "a");
for (ref_ptr = ref_start-> next; ref_ptr != NULL; ref_ptr = ref_ptr-> next)
   {
   subtotal = 0;
   subtotal2 = 0;
   for (map_ptr = map_start; map_ptr != NULL; map_ptr = map_ptr-> next)
      {
      if (map_ptr-> ref == ref_ptr)
         {
         subtotal2 += map_ptr-> size;
         if (map_ptr-> used)
            subtotal += map_ptr-> size;
         }
      }

   if (subtotal2 != 0)
      {
      filetotal += subtotal;
      filetotal2 += subtotal2;
      total += subtotal;
      total2 += subtotal2;
      (void)fprintf (stdout, "%s %8d %8d %8d %8d %8d %s\n",
         FlagOutput("item", ref_ptr->call, subtotal2, ref_ptr-> used, subtotal),
         ref_ptr-> call, subtotal2, 
         ref_ptr-> used, subtotal, ref_ptr-> line, ref_ptr-> file);
      }

   if (ref_ptr-> flag == LastFileEntry && filetotal2 != 0)
      {
      (void)fprintf (stdout, 
         "%s %s %8d %s %8d          *************** %s (total)\n",
         FlagOutput("subtotal", 0, 0, 0, 0),
         space, filetotal2, space, filetotal, ref_ptr-> file);
      filetotal = 0;
      filetotal2 = 0;
      }
   }
(void)fprintf (stdout, 
   "%s %s %8d %s %8d          *************** (grand total)\n", 
   FlagOutput("total", 0, 0, 0, 0),
   space, total2, space, total);

#ifdef MEMMAP
(void)fprintf(stderr, "m");
for (map_ptr = map_start; map_ptr != NULL; map_ptr = map_ptr-> next)
#ifdef MEMMAPUSED
   if (!map_ptr-> used)
#endif
      (void) fprintf(stdout, "%s %5d %c %s %d %d\n", 
         map_ptr-> addr, map_ptr-> size, map_ptr-> type, 
         map_ptr-> ref-> file,  map_ptr-> ref-> line, map_ptr-> used);
#endif
(void)fprintf(stderr, "\n");

} /* end of main */
/*
 * AddReference
 *
 */

static Reference * AddReference(p, line, file, inorder)
Reference * p;
unsigned int line;
char *       file;
int          inorder;
{
Reference * q;
Reference * r;
MatchStatus found = NotFound;
int         diff;

for (r = q = p; q != NULL; r = q, q = q-> next)
   {
   diff = strcmp(q-> file, file);
   if (diff == 0)
      {
      if (q-> line == line)
         found = FoundExactMatch;
      else
         if (inorder && (q-> line > line))
            found = FoundGreaterLine;
         else
            ;
      }
   else
      {
      if (inorder && (diff > 0))
         found = FoundGreaterFile;
      else
         ;
      }
   if (found != NotFound)
      break;
   }

if (found == FoundExactMatch)
   {
   q-> used++;
   q-> call++;
   }
else
   {
   p = q;
   q = (Reference *) malloc(sizeof(Reference));
   if (q == NULL)
      (void)fprintf(stderr,"out of memory at %d\n", __LINE__);
   q-> file = strdup(file);
   q-> line = line;
   q-> used = 1;
   q-> call = 1;
   switch(found)
      {
      case NotFound:
      case FoundGreaterFile:
         q-> flag = LastFileEntry;
         if (strcmp(r-> file, q-> file) == 0)
            r-> flag = SameFile;
         break;
      case FoundGreaterLine:
         q-> flag = SameFile;
         break;
      }
   q-> next = p;
   r-> next = q;
   }

return (q);

} /* end of AddReference */
/*
 * Match
 *
 */

static void Match(p)
MemoryMap * p;
{
MemoryMap * q;

for (q = p-> next; q != NULL; q = q-> next)
   if ((DEALLOCATION(q) || REALLOCATION(q)) && strcmp(q-> addr, p-> addr) == 0)
      break;

if (q != NULL)
   {
   if (REALLOCATION(q))
      {
      p-> ref-> used--;
      p-> used = FALSE;
      }
   else
      {
      p-> ref-> used--;
      q-> ref-> used--;
      q-> used = p-> used = FALSE;
      }
   }

} /* end of Match */
