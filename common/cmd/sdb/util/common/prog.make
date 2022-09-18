#ident	"@(#)sdb:util/common/prog.make	1.1"

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(CPLUS) -o $(TARGET) $(LINK_MODE) $(OBJECTS) $(LIBRARIES) $(LDLIBS)

install:	$(CCSBIN)/$(BASENAME)

$(CCSBIN)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSBIN)/$(BASENAME)
