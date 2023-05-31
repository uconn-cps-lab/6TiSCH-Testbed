# include the top level makefile setting
#
include Makefile.inc

#
# sub project specific setting
#
#PRJCFLAGS += -DG_9903

CFLAGS = $(PRJCFLAGS) -c

OBJLIB = $(LIBDIR)/comm.a
OBJDIR = $(TOPDIR)/proj/comm
SRCDIR = $(TOPSRCDIR)/comm
ifeq ($(DTLSON),true)
SRCDIR2 = $(TOP_LEVEL_PROJ_DIR)/Components/dtls
SRCDIR3 = $(TOP_LEVEL_PROJ_DIR)/Components/apps/dtls
SRCDIR4 = $(SRCDIR3)/aes
SRCDIR5 = $(SRCDIR3)/sha2
SRCDIR6 = $(SRCDIR3)/ecc
endif

SOURCES = comm.c uart.c tun.c nm.c device.c
ifeq ($(DTLSON),true)
SOURCES2 = dtls_client.c \
ccm.c \
crypto.c \
debug.c \
dtls.c \
dtls_time.c \
hmac.c \
netq.c \
peer.c \
session.c \
rijndael.c \
sha2.c \
ecc.c
endif

OBJS := $(SOURCES:%.c=$(OBJDIR)/%.o)
ifeq ($(DTLSON),true)
OBJS+= $(SOURCES2:%.c=$(OBJDIR)/%.o)
endif

DEPFILES := $(SOURCES:%.c=$(OBJDIR)/%.d)

ifeq ($(DTLSON),true)
DEPFILES += $(SOURCES2:%.c=$(OBJDIR)/%.d)
endif

all: $(OBJLIB) 

$(OBJLIB) : $(OBJS)
	$(AR) $(ARFLAGS) rv $(OBJLIB) $?
	$(RANLIB) $(OBJLIB)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILES)
endif

ifeq ($(DTLSON),true)
$(OBJDIR)/%.o : $(SRCDIR6)/%.c  $(OBJDIR)/%.d 
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR6)/%.c 
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/uip\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;
	
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
 endif
   
$(OBJDIR)/%.o : $(SRCDIR)/%.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	$(CC)  -M $(CFLAGS) $< > $@.$$$$; \
	$(SED) '1s/.*.o/comm\/&/g' < $@.$$$$ > $@_.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@_.$$$$ > $@; \
	$(RM) -f $@.$$$$; \
	$(RM) -f $@_.$$$$;

clean:
	$(RM) -f $(OBJDIR)/*.o
	$(RM) -f $(OBJDIR)/*.d
	$(RM) -f $(LIBDIR)/*.a
