########################################################
#  toolchain config
########################################################

ifeq ($(filter y,$(LOCAL) $(local)),y)
PREFIX=
else
PREFIX=arm-linux-gnueabihf-
endif

########################################################
#  MAC64 config
########################################################
ifeq ($(filter y,$(MAC64) $(mac64)),y)
MAC64_CONFIG= -DTSCH_MAC_USE_MAC64 
else
MAC64_CONFIG=
endif

########################################################
# default configuration
########################################################
AES=y
CRC16_MSB=y
RFC6282=y
ROUTING_PROTO=LOAD_NG
LBPHUT=n
DBG=y
CLEAN=n
########################################################
#
########################################################
MACSECON=true

DTLSON=false
DTLSTEST=false

ifeq ($(filter y,$(DTLS) $(dtls)),y)
DTLSON=true
endif

ifeq ($(filter y,$(DTLSTEST) $(dtlstest)),y)
DTLSON=true
DTLSTEST=true
endif

TOP_LEVEL_PROJ_DIR=../../..

DTLS_INC1_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/dtls
DTLS_INC2_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/dtls/aes
DTLS_INC3_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/dtls/ecc
COMMON_INC_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/inc
GATEWAY_INC_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/gateway/include
UIP_INC_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/uip/inc
UIP_CONF_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/uip/conf
UIP_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/uip
LINUX_PLTFRM=$(TOP_LEVEL_PROJ_DIR)/Components/uip/uip_rpl_impl/linux
RPL_ATTRIBUTE=$(TOP_LEVEL_PROJ_DIR)/Components/uip/uip_rpl_impl
APPS_INC_DIR=$(TOP_LEVEL_PROJ_DIR)/Components/apps

TOPSRCDIR = $(TOP_LEVEL_PROJ_DIR)/Components/gateway
APPSDIR = $(TOP_LEVEL_PROJ_DIR)/Components/apps
LIBDIR = $(TOP_LEVEL_PROJ_DIR)/Projects/gateway/lib
TOPDIR = ..
PROJDIR = $(TOPDIR)/proj

# BBB project tokens
PRJDEF = -DBORDER_ROUTER=1 -DLINUX_GATEWAY -DUIP_SMARTNET -DIS_ROOT=1 -DWITH_UIP6=1 -DWITH_ULPSMAC=1 -DUIP_CONF_TCP=0 

ifeq ($(MACSECON),true)
PRJDEF +=-DFEATURE_MAC_SECURITY=1
endif

ifeq ($(DTLSON),true)
PRJDEF +=-DWITH_SHA256=1 -DDTLS_WITH_TIRTOS=1 -DDTLSv12 -DDTLS_CHECK_CONTENTTYPE
PRJDEF +=-DFEATURE_DTLS=1 -DUSE_SOFTWARE_ENCRYPTION=1
endif

ifeq ($(DTLSTEST),true)
PRJDEF +=-DFEATURE_DTLS_TEST=1
endif

# BBB project flags
PRJCFLAGS =  -I$(COMMON_INC_DIR) -I$(GATEWAY_INC_DIR) -I$(UIP_INC_DIR) -I$(UIP_CONF_DIR) -I$(UIP_DIR) -I$(LINUX_PLTFRM) -I$(APPS_INC_DIR)
ifeq ($(DTLSON),true)
PRJCFLAGS += -I$(DTLS_INC1_DIR) -I$(DTLS_INC2_DIR) -I$(DTLS_INC3_DIR)
endif

PRJCFLAGS += $(PRJDEF)

PRJCFLAGS += $(MAC64_CONFIG)

PRJCFLAGS += -g -funsigned-char -std=gnu99

#
# Toolchain for projects
#
CC = $(PREFIX)gcc
LD = $(PREFIX)gcc
AR = $(PREFIX)ar
STRIP = $(PREFIX)strip
ARFLAGS = 
RANLIB = $(PREFIX)ranlib
RM = rm
SED = sed
ECHO = echo

#
# GW software application
#
EXE = gw_app.exe
