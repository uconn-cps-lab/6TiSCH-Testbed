include Makefile.inc

ifeq ($(DBG),y)
PRJCFLAGS += -g
endif

ifeq ($(LBPHUT),y)
PRJCFLAGS += -ADP_LBP_HOST_UT
endif


EXE = gwapp.exe

CFLAGS = $(PRJCFLAGS) -c 
OBJDIR = $(TOPDIR)/proj/main/
SRCDIR = $(TOPDIR)/gw-app
INSTDIR = $(TOPDIR)/bin

all : $(EXE)

OBJS = $(TOPDIR)/proj/main/gw-app.o
          
SUBDIRS = \
          $(PROJDIR)/util \
          $(PROJDIR)/comm \
          $(PROJDIR)/uip \
          $(PROJDIR)/main \
		  $(PROJDIR)/thread \
		  $(PROJDIR)/apps 
         
LIBS = \
       $(LIBDIR)/util.a \
       $(LIBDIR)/comm.a \
       $(LIBDIR)/uip.a \
       $(LIBDIR)/thread.a \
       $(LIBDIR)/apps.a

$(EXE) : $(OBJS) $(LIBS)
	$(LD) -o $(EXE) $(OBJS) $(LIBS) $(LIBS) -lpthread -lrt

ifeq ($(DBG),n)
	$(STRIP) -s $(EXE)
endif

$(LIBDIR)/util.a: force
	make -f $(PROJDIR)/util/Makefile		

$(LIBDIR)/comm.a: force
	make -f $(PROJDIR)/comm/Makefile

$(LIBDIR)/uip.a: force
	make -f $(PROJDIR)/uip/Makefile


$(LIBDIR)/thread.a: force
	make -f $(PROJDIR)/thread/Makefile

$(LIBDIR)/main.a: force
	make -f $(PROJDIR)/main/Makefile

$(LIBDIR)/apps.a: force
	make -f $(PROJDIR)/apps/Makefile   

$(OBJS) : force
	make -f $(PROJDIR)/main/Makefile

force :
	true

install :
	cp $(EXE) $(INSTDIR)

clean :
	for dir in $(SUBDIRS); do \
		make -f $$dir/Makefile clean; \
	done
	
	-$(RM) -f $(OBJDIR)/*.o
	-$(RM) -f $(LIBDIR)/*.a
	-$(RM) -f $(EXE)

