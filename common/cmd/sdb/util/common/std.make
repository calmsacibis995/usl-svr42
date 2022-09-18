#ident	"@(#)sdb:util/common/std.make	1.2"

include ../../util/$(CPU)/mach.defs
include ../../util/common/defs.make
include	../../util/common/CC.rules

SOURCES = $(CSOURCES) $(CCSOURCES)

clean:
	-rm -f *.o y.* lex.yy.c

clobber:	clean
	rm -f $(TARGET)

basedepend:
	rm -f BASEDEPEND OBJECT.list
	@if [ "$(CCSOURCES)" ] ;\
		then echo "	../../util/common/depend $(CPLUS_CMD_FLAGS) $(CCSOURCES) >> BASEDEPEND" ; \
		CC=$(CC) ../../util/common/depend $(CPLUS_CMD_FLAGS) $(CCSOURCES) >> BASEDEPEND ; \
	fi
	@if [ "$(CSOURCES)" ] ;\
		then echo "	../../util/common/depend $(CC_CMD_FLAGS) $(CSOURCES) >> BASEDEPEND" ; \
		CC=$(CC) ../../util/common/depend $(CC_CMD_FLAGS) $(CSOURCES) >> BASEDEPEND ; \
	fi
	chmod 666 BASEDEPEND

depend:	basedepend
	rm -f DEPEND
	cat BASEDEPEND | \
		../../util/common/substdir $(PRODINC) '$$(PRODINC)' | \
		../../util/common/substdir $(SGSBASE) '$$(SGSBASE)' | \
		../../util/common/substdir $(INCC) '$$(INCC)' | \
		../../util/common/substdir $(INC) '$$(INC)' > DEPEND
	../../util/common/mkdefine OBJECTS < OBJECT.list >> DEPEND
	chmod 444 DEPEND
	rm -f BASEDEPEND

rebuild:	clobber depend all
