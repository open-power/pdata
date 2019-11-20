AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -Im4
AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = -Wall -Werror $(ENDIAN_FLAG)

if BUILD_TOOLS
data_DATA = witherspoon.dtb
endif
EXTRA_PROGRAMS =

noinst_LTLIBRARIES =
lib_LTLIBRARIES =

nobase_include_HEADERS =

M4_V = $(M4_V_$(V))
M4_V_ = $(M4_V_$(AM_DEFAULT_VERBOSITY))
M4_V_0 = @echo "  M4      " $@;

DTC_V = $(DTC_V_$(V))
DTC_V_ = $(DTC_V_$(AM_DEFAULT_VERBOSITY))
DTC_V_0	= @echo "  DTC     " $@;

%.dts: dts/%.dts.m4
	$(M4_V)$(M4) -I$(srcdir)/dts $< | $(DTC) -I dts -O dts > $@

%.dtb: %.dts
	$(DTC_V)$(DTC) -i$(dir $@) -I dts $< -O dtb > $@
