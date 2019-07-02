AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -Im4
AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = -Wall -Werror $(ENDIAN_FLAG)

if BUILD_TOOLS
bin_PROGRAMS = attributes
endif
EXTRA_PROGRAMS = attributes

noinst_LTLIBRARIES = libcommon.la
lib_LTLIBRARIES = libatdb.la

libcommon_la_SOURCES = \
	ccan/build_assert/build_assert.h \
	ccan/hash/hash.h \
	ccan/hash/hash.c \
	common/dlinklist.h \
	common/dtree.c \
	common/dtree.h \
	common/mmap_file.c \
	common/mmap_file.h

libatdb_la_SOURCES = \
	atdb/atdb.c \
	atdb/atdb.h \
	atdb/atdb_blob.c \
	atdb/atdb_blob.h \
	atdb/atdb_io.c \
	atdb/atdb_io.h \
	atdb/atdb_lowlevel.c
libatdb_la_LIBADD = libcommon.la

attributes_SOURCES = \
	tools/attribute.c \
	tools/attribute.h \
	tools/translate.c \
	tools/translate.h
attributes_LDADD = libatdb.la
