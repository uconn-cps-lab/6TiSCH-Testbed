# include the top level makefile setting
#
include Makefile.inc

#
# sub project specific setting
#
#PRJCFLAGS += -DG_9903

CFLAGS = $(PRJCFLAGS) -c

OBJLIB = $(LIBDIR)/thread.a
OBJDIR = $(TOPDIR)/proj/thread
SRCDIR = $(LINUX_PLTFRM)
SRCDIR2 = $(RPL_ATTRIBUTE)

SOURCES = uip_rpl_process.c

SOURCES2 = uip_rpl_attributes.c

OBJS := $(SOURCES:%.c=$(OBJDIR)/%.o)
OBJS+= $(SOURCES2:%.c=$(OBJDIR)/%.o)

DEPFILES = $(SOURCES:%.c=$(OBJDIR)/%.d)

all: $(OBJLIB) 

$(OBJLIB) : $(OBJS)
	$(AR) $(ARFLAGS) rv $(OBJLIB) $?
	$(RANLIB) $(OBJLIB)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILES)
endif

$(OBJDIR)/%.o : $(SRCDIR2)/%.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR2)/%.c
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/thread\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/thread\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

clean:
	$(RM) -f $(OBJDIR)/*.o
	$(RM) -f $(OBJDIR)/*.d
	$(RM) -f $(LIBDIR)/*.a
