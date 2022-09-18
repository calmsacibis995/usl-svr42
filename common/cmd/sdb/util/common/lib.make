#ident	"@(#)sdb:util/common/lib.make	1.1"

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(AR) -qc $(TARGET) $(OBJECTS)
	chmod 664 $(TARGET)

all:	$(TARGET)

install:	all
