# include the top level makefile setting
#
include Makefile.inc

#
# sub project specific setting
#
#PRJCFLAGS += -DG_9903

CFLAGS = $(PRJCFLAGS) -c

OBJLIB = $(LIBDIR)/util.a
OBJDIR = $(TOPDIR)/proj/util
SRCDIR = $(TOPSRCDIR)/util

SOURCES = util.c \
    crc16.c \
	bm_api.c \
	pltfrm_lib.c \
	pltfrm_mbx.c \
	pltfrm_timer.c 

OBJS := $(SOURCES:%.c=$(OBJDIR)/%.o)

DEPFILES = $(SOURCES:%.c=$(OBJDIR)/%.d)

all: $(OBJLIB) 

$(OBJLIB) : $(OBJS)
	$(AR) $(ARFLAGS) rv $(OBJLIB) $?
	$(RANLIB) $(OBJLIB)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILES)
endif

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/util\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

clean:
	$(RM) -f $(OBJDIR)/*.o
	$(RM) -f $(OBJDIR)/*.d
	$(RM) -f $(LIBDIR)/*.a
