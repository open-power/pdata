AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -Im4
AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = -Wall -Werror $(ENDIAN_FLAG)

lib_LTLIBRARIES =
datadir = ${datarootdir}/pdata
data_DATA =
CLEANFILES =

if BUILD_LIBS
lib_LTLIBRARIES += libfdt-traverse.la libfdt-attr.la
endif

if BUILD_TOOLS
bin_PROGRAMS = attributes
data_DATA += p9.dtb p10.dtb
noinst_LTLIBRARIES = libdtm.la
CLEANFILES += p9.dtb p10.dtb
endif
EXTRA_PROGRAMS = attributes

if BUILD_ATTR_API
pkgconfiglibdir = ${libdir}/pkgconfig
pkgconfiglib_DATA = libdt-api/libdt-api.pc
lib_LTLIBRARIES += libdt-api.la
include_HEADERS = attributes_info.H libdt-api/dt_api.H
data_DATA += attributes_info.db
CLEANFILES += attributes_info.H attributes_info.db
endif

if BUILD_DYNAMIC_DT
data_DATA += power-target.dtb
CLEANFILES += power-target.dts power-target.dtb
endif

libfdt_traverse_la_SOURCES = \
	fdt/fdt_error.c \
	fdt/fdt_error.h \
	fdt/fdt_traverse.c \
	fdt/fdt_traverse.h

libfdt_attr_la_SOURCES = \
	fdt/fdt_attr.c \
	fdt/fdt_attr.h \
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
	libdtm/dtm_file.c \
	libdtm/dtm_internal.h \
	libdtm/dtm_io.c \
	libdtm/dtm_node.c \
	libdtm/dtm_nodelist.c \
	libdtm/dtm_property.c \
	libdtm/dtm_search.c \
	libdtm/dtm_traverse.c \
	libdtm/dtm_tree.c
libdtm_la_LIBADD = libfdt-traverse.la libfdt-attr.la

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

# attribute_info.H and attribute_info.db cannot be built in parallel
.NOTPARALLEL:

attributes_info.H:
	$(srcdir)/scripts/dt_generate.sh header $@

attributes_info.db:
	$(srcdir)/scripts/dt_generate.sh infodb $@

libdt_api_la_SOURCES = libdt-api/dt_api.C \
                       libdt-api/dt_api.H

DTC_V = $(DTC_V_$(V))
DTC_V_ = $(DTC_V_$(AM_DEFAULT_VERBOSITY))
DTC_V_0	= @echo "  DTC     " $@;

power-target.dts:
	$(srcdir)/scripts/dt_generate.sh dts $@

power-target.dtb: power-target.dts
	$(DTC_V)$(DTC) -I dts -O dtb $< > $@

%.dtb: dts/%.dts
	$(DTC_V)$(DTC) -i$(dir $@) -I dts $< -O dtb > $@
