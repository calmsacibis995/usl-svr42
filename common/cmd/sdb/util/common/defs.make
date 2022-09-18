#ident	"@(#)sdb:util/common/defs.make	1.2"

CPLUS	= ../../util/$(CPU)/CC
CCFLAGS	= $(CFLAGS)
LDLIBS	= -lw
INSDIR	= $(CCSBIN)

SGSBASE	= $(PRODDIR)/../sgs
COMINC	= $(SGSBASE)/inc/common
CPUINC = $(SGSBASE)/inc/$(CPU)

PRODDIR = ../..

PRODLIB	= $(PRODDIR)/lib
PRODINC	= $(PRODDIR)/inc
INCCOM	= $(PRODDIR)/inc/common
INCCPU	= $(PRODDIR)/inc/$(CPU)
COMMON	= ../common

INCLIST	= -I. -I$(COMMON) -I$(INCCPU) -I$(INCCOM) -I$(CPUINC) -I$(COMINC)

DFLAGS	=

CC_CMD_FLAGS = $(CFLAGS) $(INCLIST) -I$(INC) $(DEFLIST) $(DFLAGS)
CC_CMD	= $(CC) $(CC_CMD_FLAGS)

CPLUS_CMD_FLAGS = $(CCFLAGS) $(INCLIST) -I$(INC) $(DEFLIST) $(DFLAGS)
CPLUS_CMD = $(CPLUS) $(CPLUS_CMD_FLAGS)

ARFLAGS	= qc
YFLAGS	= -ld

# The default target is used if no target is given; it must be first.

default: all
