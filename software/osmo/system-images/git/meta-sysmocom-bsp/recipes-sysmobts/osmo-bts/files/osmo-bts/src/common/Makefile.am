AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(LIBOSMOCODEC_CFLAGS)
LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOTRAU_LIBS) $(LIBOSMOCODEC_LIBS)

if ENABLE_LC15BTS
AM_CFLAGS += -DENABLE_LC15BTS
endif

noinst_LIBRARIES = libbts.a libl1sched.a
libbts_a_SOURCES = gsm_data_shared.c sysinfo.c logging.c abis.c oml.c bts.c \
		   rsl.c vty.c paging.c measurement.c amr.c lchan.c \
		   load_indication.c pcu_sock.c handover.c msg_utils.c \
		   tx_power.c bts_ctrl_commands.c bts_ctrl_lookup.c \
		   l1sap.c cbch.c power_control.c main.c phy_link.c \
		   dtx_dl_amr_fsm.c scheduler_mframe.c

libl1sched_a_SOURCES = scheduler.c
