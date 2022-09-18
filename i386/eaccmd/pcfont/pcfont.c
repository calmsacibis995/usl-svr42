/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/pcfont/pcfont.c	1.1"
#ident	"Header: $"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include "pcfont.h"

/*  Extensions for user font description files  */
char *ext[4] = {"8x8", "8x14", "8x16", "9x16"};

/*  Number of 'bytes' in each area  */
int num_bytes[NUM_FONTS] = {SIZE_8x8, SIZE_8x14, SIZE_8x16, SIZE_9x16};

int line_num;		/*  Line number in file for error messages  */

/*  Table used for working out where to put the user defined characters  */
unsigned char rom_table[NUM_ASCII];

FILE *config_fp;	/*  Pointer to configuration file  */

struct scrn_dflt def_scrnmap;

rom_font_t font_map;		/*  Font structure for ioctl  */

/**
 *  Program to download font description to VGA/EGA graphics card.
 *  Two input formats are supported, Adobe BDF (text) format and 
 *  proprietary USL binary format.  The USL format will download all
 *  supported font sizes (8x8, 8x14, 8x16 and 9x16).  The BDF will
 *  only download the font size specified in the file.
 **/
main(argc, argv)
int argc;
char *argv[];
{
	register int i;			/*  Loop variable  */
	register int fnt_ct;	/*  Count of the number of new characters  */
	register int hash_val;	/*  Hash function value  */
	register int old_hash;	/*  remembered value of the hash value  */
	int con_fd;				/*  Console file descriptor  */
	int vid_fd;				/*  /dev/video file descriptor  */
	int tty_fd;				/*  /dev/tty file descriptor  */
	int found;				/*  Flag to indicate the character is found  */
	int index;				/*  Position in table for character  */
	int start;				/*  Starting point of search  */
	int file_load = 0;		/*  Format of input file  */

	char sym_name[SYM_LEN];	/*  Symbolic name of character  */
	char tmp_name[GEN_LEN];	/*  Full pathname of config file  */

	/*  Check that we have access to the console for the screen remap
	 *  ioctl.
	 */
	if ((con_fd = open(CONSOLE, O_RDONLY)) < 0)
	{
		fprintf(stderr, "pcfont: Unable to access the console\n");
		exit(ERROR);
	}

	if ((tty_fd = open(TTY_DEV, O_RDONLY)) >= 0)
	{
		/*  Check that this is a valid terminal to run pcfont on  */
		if (ioctl(tty_fd, KIOCINFO) < 0)
		{
			fprintf(stderr, "pcfont can only be run from a virtual terminal\n");
			fprintf(stderr, "on a graphics workstation\n");
			exit(ERROR);
		}
	}

	/*  Check that we can access /dev/video for the font download ioctl
	 */
	if ((vid_fd = open(VIDEO, O_RDONLY)) < 0)
	{
		fprintf(stderr, "pcfont: Unable to access %s\n", VIDEO);
		perror("pcfont");
		exit(ERROR);
	}

	/*  Reset the character mapping table before we start.  This avoids
	 *  problems with old maps being used.
	 */
	for (i = 0; i < NUM_ASCII; i++)
		def_scrnmap.scrn_map[i] = i;

	/*  Set up to copy to the kernel  */
	def_scrnmap.scrn_direction = KD_DFLTSET;

	/*  If there is no configuration file the user wants things put back
	 *  to normal.
	 */
	if (argc < 2)
	{
		/*  Reset the font map by setting the number of characters to zero
		 */
		font_map.fnt_numchar = 0;

		/*  Execute the ioctl  */
		if (ioctl(vid_fd, WS_PIO_ROMFONT, &font_map) < 0)
		{
			fprintf(stderr, "pcfont: Failed to reset font map\n");
			perror("pcfont");
			exit(ERROR);
		}

		/*  Execute the ioctl to reset the mapping  */
		if (ioctl(con_fd, KDDFLTSCRNMAP, &def_scrnmap) < 0)
		{
			fprintf(stderr, "pcfont: Failed to set screen map\n");
			perror("pcfont");
			exit(ERROR);
		}

		exit(SUCCESS);
	}

	/*  The -f option is used to indicate a BDF format font description
	 *  file.  If no -f is used, we load an SVR4 format font file.  These 
	 *  are binary format and download all font sizes, which the BDF ones
	 *  do not
	 */
	if (getopt(argc, argv, "f:") != EOF)
	{
		/*  Execute the ioctl to reset the mapping  */
		if (ioctl(con_fd, KDDFLTSCRNMAP, &def_scrnmap) < 0)
		{
			fprintf(stderr, "pcfont: Failed to set screen map\n");
			perror("pcfont");
			exit(ERROR);
		}

		load_font(optarg);

		/*  Download the new font descriptions  */
		if (ioctl(vid_fd, WS_PIO_ROMFONT, &font_map))
		{
			fprintf(stderr, "pcfont: Failed to reset font map\n");
			perror("pcfont");
			exit(ERROR);
		}
		
		exit(SUCCESS);
	}

	/*  Create full pathname of config file  */
	sprintf(tmp_name, "%s%s", FONT_DIR, argv[1]);

	/*  Now open the file
	 */
	if ((config_fp = fopen(tmp_name, "r")) == NULL)
	{
		fprintf(stderr, "pcfont: Unable to open <%s> for reading\n", argv[1]);
		exit(ERROR);
	}

	/*  Construct the hash tables for the default code set and then
	 *  the user defined characters.
	 */
	init_rom_font();
	init_u_font(argv[1]);

	line_num = 0;	/*  Clear the line count  */
	fnt_ct = 0;		/*  Clear the font count  */

	/*  Clear the rom_table  */
	for (i = 0; i < NUM_ASCII; i++)
		rom_table[i] = 0;

	/*  Process each symbolic name, index pair in the config file  */
	while (get_sym_name(sym_name, &index) != END_OF_FILE)
	{
		/*  Generate the hash function value.  This is the same for the
		 *  default table and the user defined table, which makes life
		 *  a little simpler.
		 */
		hash_val = hash_func(sym_name);

		found = TRUE;
		old_hash = hash_val;

		/*  Check to see if the character is defined in the default font
		 */
		while (strcmp(hash_table[hash_val].sym_name, sym_name) != 0)
		{
			if (++hash_val >= HASH_SIZE)	/*  Wrap round at end of table */
				hash_val = 0;

			/*  Have we gone all the way round the table or hit a blank
			 *  entry, which indicates that the character is not there ?
			 */
			if (hash_val == old_hash ||
				hash_table[hash_val].sym_name[0] == NULL)
			{
				found = FALSE;	
				break;
			}
		}

		/*  If we did find the character we record the change in the mapping
		 *  table, and continue.  We also record the index that was remapped
		 *  for later use with user defined charactrers.
		 */
		if (found == TRUE)
		{
			def_scrnmap.scrn_map[index] = hash_table[hash_val].sym_value;
			rom_table[hash_table[hash_val].sym_value] = index;
			continue;
		}

		hash_val = old_hash;		/*  Reset the hash value  */
		found = TRUE;				/*  Set the found flag  */

		/*  Now search the user defined font table.  If its not here 
		 *  then we have an error since the character is undefined.
		 */
		while (strcmp(font_table[hash_val].sym_name, sym_name) != 0)
		{
			if (++hash_val >= HASH_SIZE)	/*  Check for wrap around  */
				hash_val = 0;

			/*  Have we come all the way round or hit a blank entry?  */
			if (hash_val == old_hash ||
			    font_table[hash_val].def_map == 0)
			{
				fprintf(stderr, "pcfont: Undefined symbolic name <%s> on line %d\n",
					sym_name, line_num);
				exit(ERROR);
			}
		}

		/*  Record the information about the new character in the font 
		 *  table.
		 */
		font_map.fnt_chars[fnt_ct].cd_index = index;

		/*  Copy the bit maps from the hash table to the font structure  */
		for (i = 0; i < F8x8_BPC; i++)
			font_map.fnt_chars[fnt_ct].cd_map_8x8[i] =
			    *(font_table[hash_val].bit_map[0] + i);

		for (i = 0; i < F8x14_BPC; i++)
			font_map.fnt_chars[fnt_ct].cd_map_8x14[i] =
			    *(font_table[hash_val].bit_map[1] + i);

		for (i = 0; i < F8x16_BPC; i++)
			font_map.fnt_chars[fnt_ct].cd_map_8x16[i] =
			    *(font_table[hash_val].bit_map[2] + i);

		for (i = 0; i < F9x16_BPC; i++)
			font_map.fnt_chars[fnt_ct].cd_map_9x16[i] =
			    *(font_table[hash_val].bit_map[3] + i);

		font_map.fnt_numchar = ++fnt_ct;	/*  Bump up the character count  */
	}

	/*  Having built up the map we now need to resolve problems caused by
	 *  altering the mappings of characters.  We need to know where we can
	 *  put the newly defined characters.
	 */
	for (i = 0; i < (int)font_map.fnt_numchar; i++)
	{
		/*  If the character has not been remapped there is no problem
		 *  and we can continue.
		 */
		/*  XXX 0 is a valid value - This needs further consideration  */
		if (rom_table[font_map.fnt_chars[i].cd_index] == 0)
			continue;

		/*  Set up start for search  */
		start = rom_table[font_map.fnt_chars[i].cd_index];
		index = start;

		/**  XXX again 0 is valid  **/
		while (rom_table[index] != 0)
		{
			index = rom_table[index];

			/*  Check for an infinite loop - This can happen if the user
			 *  defines more than one character in the same place.
			 */
			if (index == start)
			{
				fprintf(stderr, "pcfont: Internal error - cannot locate space for new character.\n");
				exit(ERROR);
			}
		}

		/*  Having found the slot we need to update the maps  */
		def_scrnmap.scrn_map[font_map.fnt_chars[i].cd_index] = index;
		rom_table[index] = font_map.fnt_chars[i].cd_index;
		font_map.fnt_chars[i].cd_index = index;
	}

	/*  Download the new font descriptions  */
	if (ioctl(vid_fd, WS_PIO_ROMFONT, &font_map))
	{
		fprintf(stderr, "pcfont: Failed to reset font map\n");
		perror("pcfont");
		exit(ERROR);
	}

	/*  Set up the default mapping structure  */
	def_scrnmap.scrn_direction = KD_DFLTSET;

	/*  Alter the character map as required  */
	if (ioctl(con_fd, KDDFLTSCRNMAP, &def_scrnmap) < 0)
	{
		fprintf(stderr, "pcfont: Failed to reset screen map\n");
		perror("pcfont");
		exit(ERROR);
	}

	/*  Tidy up and exit  */
	close(vid_fd);
	close(con_fd);
	fclose(config_fp);
	exit(SUCCESS);
}

/**
 *  Function to initialise the default ROM hash table.
 *  No return value - if anything goes wrong we bomb out
 **/
void
init_rom_font()
{
	register int i;		/*  Loop variable  */
	int hash_value;		/*  Calculated hash value  */
	int sym_value;		/*  Value of the symbol  */

	char inp_line[INP_LEN];		/*  Line read from config file  */
	char sym_name[SYM_LEN];		/*  Name of symbolic name  */

	FILE *ifp;			/*  Input file pointer  */

	/*  Open the default ROM font file for reading  */
	if ((ifp = fopen(ROM_FONT, "r")) == NULL)
	{
		fprintf(stderr, "pcfont: Unable to read default font file.\n");
		exit(ERROR);
	}

	/*  Process the file, one line at a time  */
	while (fgets(inp_line, INP_LEN-1, ifp) != NULL)
	{
		/*  Ignore lines starting with a # which are comments  */
		if (inp_line[0] == COMMENT)
			continue;

		i = 0;

		/*  The first field of the line is the symbolic name  */
		while (!isspace(inp_line[i]) &&
		       inp_line[i] != NULL &&
		       inp_line[i] != RETURN)
		{
			sym_name[i] = inp_line[i];
			++i;
		}

		sym_name[i++] = NULL;

		/*  This bit should never happen, but if the ROM font file 
		 *  becomes corrupted this will prevent the user from losing
		 *  the font on the console, which could have undesired
		 *  affects.  (Depression, Suicide etc).
		 */
		if (!isdigit(inp_line[i]))
		{
			fprintf(stderr, "pcfont: The default ROM font file is corrupted\n");
			exit(ERROR);
		}

		sym_value = atoi(&inp_line[i]);
		hash_value = hash_func(sym_name);

		/*  If this slot in the hash table is already used we loop
		 *  down the table until we find an empty slot.  If we hit
		 *  the end of the table we restart at the beginning.
		 */
		while (hash_table[hash_value].sym_name[0] != NULL)
		{
			++hash_value;

			if (hash_value >= HASH_SIZE)
				hash_value = 0;
		}

		/*  Record the value in the table  */
		strcpy(hash_table[hash_value].sym_name, sym_name);
		hash_table[hash_value].sym_value = sym_value;
	}

	/*  Close the file  */
	fclose(ifp);
}

/**
 *  Function to read the user defined font files.  This extracts the 
 *  symbolic names and the bit map and puts them into the hash 
 *  table.  It also checks that each symbolic name is defined in all
 *  the font files.
 *
 *  No return value, if there's something wrong the program commits sepuka.
 **/
void
init_u_font(file_name)
char *file_name;
{
	register int i;				/*  Loop varaibles  */
	register int fn;
	register int hash_value;	/*  For hashing function  */
	int found;					/*  Flag to indicate name in table  */
	int hash_start;				/*  Start of search in hash table */
	int quit;					/*  Flag to indicate exit program  */
	int ifd;					/*  Input file pointer  */
	int *fptr;					/*  Font array pointer  */

	char font_name[GEN_LEN];	/*  Name of user font file  */
	char buf[INP_LEN];			/*  Line read from file  */
	char old_name[GEN_LEN];		/*  Name of last symbolic name  */

	/*  We have to process several files, so we use a loop to try and
	 *  make this as efficient as possible.
	 */
	for (fn = 0; fn < NUM_FONTS; fn++)
	{
		/*  construct the full font file name  */
		sprintf(font_name, "%s%s.%s", FONT_DIR, file_name, ext[fn]);

		/*  If we can't open the file we give up  */
		if ((ifd = open(font_name, O_RDONLY)) == NULL)
		{
			fprintf(stderr, "pcfont: Failed to open font file <%s>\n", font_name);
			exit(ERROR);
		}

		/*  Now proces the file one record at a time  */
		while (read(ifd, buf, SYM_LEN + num_bytes[fn]) == (SYM_LEN + num_bytes[fn]))
		{
			hash_value = hash_func(buf);

			/*  We remember the original hash value, so that if the
			 *  table fills up we can report an error, rather than 
			 *  get locked in an infinite loop, which some users 
			 *  find rather disconcerting.
			 */
			hash_start = hash_value;

			/*  If this is the first file we need to insert the
			 *  symbolic name into the hash table, otherwise we 
			 *  simply need to check that its already there.
			 */
			if (fn == 0)
			{
				while (font_table[hash_value].sym_name[0] != NULL)
				{
					++hash_value;

					/*  Flip back to the start if we reach the end  */
					if (hash_value >= HASH_SIZE)
						hash_value = 0;

					/*  Check for full table - this can only happen if the
					 *  user is REALLY dumb and defines over four times as
					 *  many characters as he can use.
					 */
					if (hash_value == hash_start)
					{
						fprintf(stderr, "pcfont: Internal error - hash table full\n");
						exit(ERROR);
					}
				}

				/*  Copy in the name  */
				strcpy(font_table[hash_value].sym_name, buf);

				/*  This is used after all the files have been scanned
				 *  to ensure that the character is defined in all 
				 *  font sizes.
				 */
				font_table[hash_value].def_map = 1;

				/*  While we're here we'll allocate the space for the
				 *  actual font map.
				 */
				if ((fptr = malloc(MEM_SIZE * sizeof(int))) == NULL)
				{
					fprintf(stderr, "pcfont: Internal error - no more memory\n");
					exit(ERROR);
				}

				/*  Set up the pointers to the individual bit maps  */
				font_table[hash_value].bit_map[0] = fptr;
				font_table[hash_value].bit_map[1] = fptr + SIZE_8x8;
				font_table[hash_value].bit_map[2] = fptr + SIZE_8x8 + SIZE_8x14;
				font_table[hash_value].bit_map[3] = fptr + SIZE_8x8 + SIZE_8x14 + SIZE_8x16;

				/*  Copy the 8x8 bit map  */
				for (i = 0; i < num_bytes[0]; i++)
					*(fptr + i) = buf[SYM_LEN + i];
			}
			else		/*  Not the first file  */
			{
				found = FALSE;

				/*  Search for the name  */
				while (font_table[hash_value].sym_name[0] != NULL)
				{
					/*  Found the name so exit the loop  */
					if (strcmp(font_table[hash_value].sym_name, buf) == 0)
					{
						found = TRUE;
						break;
					}

					++hash_value;

					/*  Flip round at end of table  */
					if (hash_value >= HASH_SIZE)
						hash_value = 0;

					/*  This is for the rare occasion when the table is
					 *  full and the symbolic name has not been defined
					 *  in a previous file.
					 */
					if (hash_value == hash_start)
						break;
				}

				/*  Report error if name not found  */
				if (found == FALSE)
				{
					fprintf(stderr, "pcfont: <%s> is not defined in all font sizes\n", buf);
					fprintf(stderr, "Undefined in %s\n", FONT_FILE_8x8);
					exit(ERROR);
				}
				else	/*  OR in the appropriate bit  */
					font_table[hash_value].def_map |= (1 << fn);

				/*  Copy the data from the record read to the hash table  */
				for (i = 0; i < num_bytes[fn]; i++)
					*(font_table[hash_value].bit_map[fn] + i) = buf[SYM_LEN + i];
			}

			/*  Remember the name for use in an error message  */
			strcpy(old_name, buf);
			continue;
		}

		close(ifd);
	}

	quit = FALSE;	/*  Make sure quit flag is set to false  */

	/*  Having completed the file scanning, we now check the table
	 *  to make sure that all characters are defined in all font sizes.
	 */
	for (i = 0; i < HASH_SIZE; i++)
	{
		/*  Ignore unused entries  */
		if (font_table[i].def_map == 0)
			continue;

		/*  We don't stop when we find the first error.  This is to 
		 *  make life a little easier for the user by reporting all 
		 *  the errors in one go.
		 */
		if (font_table[i].def_map != ALL_FONTS)
		{
			fprintf(stderr, "pcfont: <%s> is not defined in all font sizes\n",
				font_table[i].sym_name);

			/*  By default if we pick up this problem here the character
			 *  must be defined in the first file, so the next print
			 *  statement is made simpler.
			 */
			fprintf(stderr, "Undefined in: %s %s %s\n",
				font_table[i].def_map & FONT_2 ? "" : FONT_FILE_8x14,
				font_table[i].def_map & FONT_3 ? "" : FONT_FILE_8x16,
				font_table[i].def_map & FONT_4 ? "" : FONT_FILE_9x16);
			quit = TRUE;
		}
	}

	if (quit == TRUE)
		exit(ERROR);
}

/**
 *  Function to generate hash value.  Currently this uses a really simple
 *  modulus style algorithm.  This is due to the fact that I can't find a
 *  copy of Knuth.  This will need to be resolved before the final version
 **/
int
hash_func(str)
char *str;
{
	register int i;
	register int hash_val = 0;

	for (i = 0; i < strlen(str); i++)
		hash_val += *(str + i);

	return(hash_val % HASH_SIZE);
}

/**
 *  Function to extract a symbolic name, index pair from the user
 *  configuration file.
 *  Returns 0 for pair found, 1 for end-of-file.
 **/
int
get_sym_name(sym_name, index)
char *sym_name;
int *index;
{
	register int i;				/*  Loop variables  */
	register int j;

	char inp_line[INP_LEN];		/*  Line read from file  */
	register char *ptr;			/*  Working pointer  */

	while (fgets(inp_line, INP_LEN-1, config_fp) != NULL)
	{
		/*  Bump up the line number  */
		++line_num;

		/*  If the last character is not a newline we have not read in the
		 *  whole line.  This is an error since information can get lost.
		 */
		if (inp_line[strlen(inp_line)-1] != RETURN)
		{
			fprintf(stderr, "pcfont: Line %d is too long\n", line_num);
			exit(ERROR);
		}

		/*  Skip blank lines and comments  */	
		if (inp_line[0] == COMMENT || inp_line[0] == RETURN)
			continue;

		i = 0;
		ptr = sym_name;

		/*  If the first character is not a digit or a letter then
		 *  somebody will have to pay.
		 */
		if (!isdigit(inp_line[0]) && !isalpha(inp_line[0]))
		{
			fprintf(stderr, "pcfont: Syntax error on line %d\n", line_num);
			exit(ERROR);
		}

		/*  Work up to a space or RETURN  */
		while (!isspace(inp_line[i]) && inp_line[i] != RETURN)
		{
			/*  Check for invalid characters in the symbolic name  */
			if (!isalpha(inp_line[i]) && 
			    inp_line[i] != UNDER_SCORE)
			{
				fprintf(stderr, "pcfont: Illegal symbolic name on line %d\n",
					line_num);
				exit(ERROR);
			}

			*ptr++ = inp_line[i++];
		}

		*ptr = NULL;

		/*  Skip past the space between the symbolic name and the index  */
		while (isspace(inp_line[i]))
			++i;

		/*  Check for end of line, which is well out of order  */
		if (inp_line[i] == RETURN)
		{
			fprintf(stderr, "pcfont: No character code specified on line %d\n", line_num);
			exit(ERROR);
		}

		/*  Now scan the remaining part of the string to make sure we
		 *  only have valid characters.
		 */
		for (j = i; j < strlen(inp_line) - 1; j++)
		{
			if (!isdigit(inp_line[j]) && !isspace(inp_line[j]))
			{
				fprintf(stderr, "pcfont: Non-numeric character in index on line %d\n",
					line_num);
				exit(ERROR);
			}
		}

		/*  Record the index  */
		*index = atoi(&inp_line[i]);
		return(SUCCESS);
	}

	return(END_OF_FILE);
}
