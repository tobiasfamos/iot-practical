CONTIKI_PROJECT = cast_cli
TARGET=sky
all: $(CONTIKI_PROJECT)

APPS = serial-shell 
CONTIKI = ../..


DEFINES=NETSTACK_CONF_RDC=nullrdc_driver,NETSTACK_CONF_MAC=csma_driver

include $(CONTIKI)/Makefile.include
-include /home/user/nes/testbed-scripts/Makefile.include

