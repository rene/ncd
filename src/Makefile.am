AM_CFLAGS= -Wall

bin_PROGRAMS = ncd
ncd_SOURCES = ncd.c mat.c fileop.c doncd.c compressors.c zlib.c bzlib.c \
				ppmd/Alloc.c ppmd/CpuArch.c ppmd/export.c ppmd/Ppmd7.c ppmd/Ppmd7Enc.c
ncd_LDADD=$(ZLIB_LIBS) $(BZLIB_LIBS)

