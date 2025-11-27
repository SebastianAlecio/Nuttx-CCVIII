############################################################################
# apps/examples/my_client/Makefile
############################################################################

include $(APPDIR)/Make.defs

MAINSRC = my_client_main.c

PROGNAME = my_client

PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_EXAMPLES_MY_CLIENT_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_MY_CLIENT)

CSRCS = $(MAINSRC)

include $(APPDIR)/Application.mk
