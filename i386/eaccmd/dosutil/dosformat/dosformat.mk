#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/dosformat.mk	1.5.1.2"
#ident  "$Header: dosformat.mk 1.2 91/07/08 $"

include $(CMDRULES)


OWN = bin
GRP = dos

DIRS = $(USRBIN) $(ETC)/default

LIB = libdos.a
LDLIBS = $(LIB)
DOSOBJECTS = $(CFILES:.c=.o)
CMDS = dosformat 

.MUTEX: $(LIB) $(CMDS)


all: $(LIB) $(CMDS)

CFILES=MS-DOS.c \
	add_device.c \
	alloc_clust.c \
	basename.c \
	chain_clust.c \
	close_device.c \
	critical.c \
	del_label.c \
	dos_fil_size.c \
	dos_mod_date.c \
	dos_mod_time.c \
	fix_slash.c \
	free_space.c \
	get_assign.c \
	is_dir_empty.c \
	loc_free_dir.c \
	locate.c \
	lookup_dev.c \
	lookup_drv.c \
	make_label.c \
	mkdir.c \
	my_fgets.c \
	next_cluster.c \
	next_sector.c \
	open_device.c \
	parse_name.c \
	read_sector.c \
	rm_file.c \
	scan_dos_dir.c \
	strupr.c \
	write_fat.c \
	write_sector.c

dosformat: $(LIB) dosformat.o
	$(CC) -o dosformat dosformat.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dosformat.o: dosformat.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	MS-DOS.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h \
	MS-DOS_boot.h

libdos.a: $(DOSOBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $?

$(DOSOBJECTS): MS-DOS.h

install: $(DIRS) all
	 $(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosformat
	
$(DIRS):
	[ -d $@ ] || mkdir $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chown $(OWN) $@ ;\
		$(CH)chgrp bin $@

clean:
	rm -f *.o $(DOSOBJECTS)

clobber: clean
	rm -f $(CMDS) $(LIB)

lintit:
	$(LINT) $(LINTFLAGS) *.c
