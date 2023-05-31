# include the top level makefile setting
#
include Makefile.inc

#
# sub project specific setting
#
#PRJCFLAGS += -DG_9903

CFLAGS = $(PRJCFLAGS) -c -DCOAP_MESSAGING_MOD

OBJLIB = $(LIBDIR)/apps.a
OBJDIR = $(TOPDIR)/proj/apps
SRCDIR = $(APPSDIR)/coap
SRCDIR2 = $(APPSDIR)/json

SOURCES = async.c block.c coap_list.c \
	debug.c encode.c hashkey.c \
	net.c option.c pdu.c \
	resource.c str.c subscribe.c \
	uri.c coap_nvm.c coap_common_app.c coap_gw_app.c harp.c

SOURCES2 = jsonparse.c jsontree.c

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
	$(SED) '1s/.*.o/apps\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/apps\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

clean:
	$(RM) -f $(OBJDIR)/*.o
	$(RM) -f $(OBJDIR)/*.d
	$(RM) -f $(LIBDIR)/*.a
