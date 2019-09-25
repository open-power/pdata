AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -Im4
AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = -Wall -Werror $(ENDIAN_FLAG)

if BUILD_TOOLS
bin_PROGRAMS = attributes
data_DATA = witherspoon.dtb
check_PROGRAMS = attribute_format_test
endif
EXTRA_PROGRAMS = attributes

noinst_LTLIBRARIES = libfdt-traverse.la libdtm.la
lib_LTLIBRARIES = libfdt-prop.la

nobase_include_HEADERS = fdt/fdt_prop.h

libfdt_traverse_la_SOURCES = \
	fdt/fdt_error.c \
	fdt/fdt_error.h \
	fdt/fdt_traverse.c \
	fdt/fdt_traverse.h

libfdt_prop_la_SOURCES = \
	fdt/fdt_prop.c \
	fdt/fdt_prop.h

libdtm_la_SOURCES = \
	ccan/build_assert/build_assert.h \
	ccan/check_type/check_type.h \
	ccan/container_of/container_of.h \
	ccan/list/list.c \
	ccan/list/list.h \
	libdtm/dtm.c \
	libdtm/dtm.h \
	libdtm/dtm_internal.h \
	libdtm/dtm_io.c \
	libdtm/dtm_node.c \
	libdtm/dtm_nodelist.c \
	libdtm/dtm_property.c \
	libdtm/dtm_search.c \
	libdtm/dtm_traverse.c \
	libdtm/dtm_tree.c
libdtm_la_LIBADD = libfdt-traverse.la

attributes_SOURCES = \
	attribute/attribute.c \
	attribute/attribute.h \
	attribute/attribute_db.c \
	attribute/attribute_db.h \
	attribute/attribute_format.c \
	attribute/attribute_format.h \
	attribute/attribute_target.c \
	attribute/attribute_target.h \
	attribute/attribute_util.c \
	attribute/attribute_util.h
attributes_LDADD = libdtm.la
attributes_LDFLAGS = -lm

attribute_format_test_SOURCES = \
	attribute/attribute_format_test.c \
	attribute/attribute_format.c \
	attribute/attribute_format.h

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
