include $(top_srcdir)/Makefile.tool.am

include $(top_srcdir)/Makefile.flags.am

SUBDIRS = .
if VGP_X86_LINUX
SUBDIRS += x86
endif
if VGP_AMD64_LINUX
SUBDIRS += amd64
endif
if VGP_PPC32_LINUX
SUBDIRS += ppc32
endif
if VGP_PPC64_LINUX
SUBDIRS += ppc64
endif

DIST_SUBDIRS = ${VG_ARCH_ALL} .

check_PROGRAMS = memtracer_test

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS   = $(WERROR) -Winline -Wall -Wshadow -g $(AM_FLAG_M3264_PRI)
