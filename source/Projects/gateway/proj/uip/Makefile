# include the top level makefile setting
#
include Makefile.inc

#
# sub project specific setting
#
#PRJCFLAGS += -DG_9903

CFLAGS = $(PRJCFLAGS) -c

OBJLIB = $(LIBDIR)/uip.a
OBJDIR = $(TOPDIR)/proj/uip
SRCDIR = $(UIP_DIR)/net

SRCDIR2 = $(UIP_DIR)/rpl
SRCDIR3 = $(UIP_DIR)/sys
SRCDIR4 = $(UIP_DIR)/rime
SRCDIR5 = $(UIP_DIR)/lib

#
#  source files
#
SOURCES = sicslowpan.c nbr-table.c packetbuf.c \
	queuebuf.c tcpip.c \
	uip6.c \
	uip-debug.c \
	uip-ds6.c \
	uip-ds6-nbr.c \
	uip-ds6-route.c \
	uip-fw.c \
	uip-icmp6.c \
	uip-nd6.c \
	uip-packetqueue.c \
	uip-split.c \
	udp-simple-socket.c \
	uip-udp-packet.c 

SOURCES2= rpl.c \
	rpl-dag.c \
	rpl-ext-header.c \
	rpl-icmp6.c \
	rpl-mrhof.c \
	rpl-of0.c \
	rpl-plusplus.c \
	rpl-routing-header.c \
	rpl-timers.c 


SOURCES3= clock.c \
	ctimer.c \
	etimer.c \
	stimer.c \
	timer.c 

SOURCES4= rimeaddr.c

SOURCES5= list.c memb.c random.c


OBJS := $(SOURCES:%.c=$(OBJDIR)/%.o)

OBJS+= $(SOURCES2:%.c=$(OBJDIR)/%.o)
OBJS+= $(SOURCES3:%.c=$(OBJDIR)/%.o)
OBJS+= $(SOURCES4:%.c=$(OBJDIR)/%.o)
OBJS+= $(SOURCES5:%.c=$(OBJDIR)/%.o)


SRCFILES = $(SRCDIR)/%.c

DEPFILES = $(SOURCES:%.c=$(OBJDIR)/%.d) 

all: $(OBJLIB) 

$(OBJLIB) : $(OBJS) 
	$(AR) $(ARFLAGS) rv $(OBJLIB) $?
	$(RANLIB) $(OBJLIB)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILES)
endif

$(OBJDIR)/%.o : $(SRCDIR5)/%.c  $(OBJDIR)/%.d 
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR5)/%.c 
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/uip\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

$(OBJDIR)/%.o : $(SRCDIR4)/%.c  $(OBJDIR)/%.d 
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR4)/%.c 
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/uip\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

$(OBJDIR)/%.o : $(SRCDIR3)/%.c  $(OBJDIR)/%.d 
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR3)/%.c 
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/uip\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;


$(OBJDIR)/%.o : $(SRCDIR2)/%.c  $(OBJDIR)/%.d 
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR2)/%.c 
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/uip\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;


$(OBJDIR)/%.o : $(SRCDIR)/%.c  $(OBJDIR)/%.d 
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c 
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/uip\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;


clean:
	$(RM) -f $(OBJDIR)/*.o
	$(RM) -f $(OBJDIR)/*.d
	$(RM) -f $(LIBDIR)/*.a
