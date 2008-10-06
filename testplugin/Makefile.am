include $(top_srcdir)/Makefile.tool.am

noinst_PROGRAMS = 
if VGP_X86_LINUX
noinst_PROGRAMS += testplugin-x86-linux vgpreload_testplugin-x86-linux.so
endif
if VGP_AMD64_LINUX
noinst_PROGRAMS += testplugin-amd64-linux vgpreload_testplugin-amd64-linux.so
endif
if VGP_PPC32_LINUX
noinst_PROGRAMS += testplugin-ppc32-linux vgpreload_testplugin-ppc32-linux.so
endif
if VGP_PPC64_LINUX
noinst_PROGRAMS += testplugin-ppc64-linux vgpreload_testplugin-ppc64-linux.so
endif
if VGP_PPC32_AIX5
noinst_PROGRAMS += testplugin-ppc32-aix5 vgpreload_testplugin-ppc32-aix5.so
endif
if VGP_PPC64_AIX5
noinst_PROGRAMS += testplugin-ppc64-aix5 vgpreload_testplugin-ppc64-aix5.so
endif

vgpreload_testplugin_x86_linux_so_SOURCES      = 
vgpreload_testplugin_x86_linux_so_CPPFLAGS     = $(AM_CPPFLAGS_X86_LINUX)
vgpreload_testplugin_x86_linux_so_CFLAGS       = $(AM_CFLAGS_X86_LINUX) $(AM_CFLAGS_PIC)
vgpreload_testplugin_x86_linux_so_DEPENDENCIES = $(LIBREPLACEMALLOC_X86_LINUX)
vgpreload_testplugin_x86_linux_so_LDFLAGS      = \
	$(PRELOAD_LDFLAGS_X86_LINUX) \
	$(LIBREPLACEMALLOC_LDFLAGS_X86_LINUX)

vgpreload_testplugin_amd64_linux_so_SOURCES      = 
vgpreload_testplugin_amd64_linux_so_CPPFLAGS     = $(AM_CPPFLAGS_AMD64_LINUX)
vgpreload_testplugin_amd64_linux_so_CFLAGS       = $(AM_CFLAGS_AMD64_LINUX) $(AM_CFLAGS_PIC)
vgpreload_testplugin_amd64_linux_so_DEPENDENCIES = $(LIBREPLACEMALLOC_AMD64_LINUX)
vgpreload_testplugin_amd64_linux_so_LDFLAGS      = \
	$(PRELOAD_LDFLAGS_AMD64_LINUX) \
	$(LIBREPLACEMALLOC_LDFLAGS_AMD64_LINUX)

vgpreload_testplugin_ppc32_linux_so_SOURCES      = 
vgpreload_testplugin_ppc32_linux_so_CPPFLAGS     = $(AM_CPPFLAGS_PPC32_LINUX)
vgpreload_testplugin_ppc32_linux_so_CFLAGS       = $(AM_CFLAGS_PPC32_LINUX) $(AM_CFLAGS_PIC)
vgpreload_testplugin_ppc32_linux_so_DEPENDENCIES = $(LIBREPLACEMALLOC_PPC32_LINUX)
vgpreload_testplugin_ppc32_linux_so_LDFLAGS      = \
	$(PRELOAD_LDFLAGS_PPC32_LINUX) \
	$(LIBREPLACEMALLOC_LDFLAGS_PPC32_LINUX)

vgpreload_testplugin_ppc64_linux_so_SOURCES      = 
vgpreload_testplugin_ppc64_linux_so_CPPFLAGS     = $(AM_CPPFLAGS_PPC64_LINUX)
vgpreload_testplugin_ppc64_linux_so_CFLAGS       = $(AM_CFLAGS_PPC64_LINUX) $(AM_CFLAGS_PIC)
vgpreload_testplugin_ppc64_linux_so_DEPENDENCIES = $(LIBREPLACEMALLOC_PPC64_LINUX)
vgpreload_testplugin_ppc64_linux_so_LDFLAGS      = \
	$(PRELOAD_LDFLAGS_PPC64_LINUX) \
	$(LIBREPLACEMALLOC_LDFLAGS_PPC64_LINUX)

vgpreload_testplugin_ppc32_aix5_so_SOURCES      = 
vgpreload_testplugin_ppc32_aix5_so_CPPFLAGS     = $(AM_CPPFLAGS_PPC32_AIX5)
vgpreload_testplugin_ppc32_aix5_so_CFLAGS       = $(AM_CFLAGS_PPC32_AIX5) $(AM_CFLAGS_PIC)
vgpreload_testplugin_ppc32_aix5_so_DEPENDENCIES = $(LIBREPLACEMALLOC_PPC32_AIX5)
vgpreload_testplugin_ppc32_aix5_so_LDFLAGS      = \
	$(PRELOAD_LDFLAGS_PPC32_AIX5) \
	$(LIBREPLACEMALLOC_LDFLAGS_PPC32_AIX5)

vgpreload_testplugin_ppc64_aix5_so_SOURCES      = 
vgpreload_testplugin_ppc64_aix5_so_CPPFLAGS     = $(AM_CPPFLAGS_PPC64_AIX5)
vgpreload_testplugin_ppc64_aix5_so_CFLAGS       = $(AM_CFLAGS_PPC64_AIX5) $(AM_CFLAGS_PIC)
vgpreload_testplugin_ppc64_aix5_so_DEPENDENCIES = $(LIBREPLACEMALLOC_PPC64_AIX5)
vgpreload_testplugin_ppc64_aix5_so_LDFLAGS      = \
	$(PRELOAD_LDFLAGS_PPC64_AIX5) \
	$(LIBREPLACEMALLOC_LDFLAGS_PPC64_AIX5)

NONE_SOURCES_COMMON = tp_main.c mem_tracer.c

testplugin_x86_linux_SOURCES      = $(NONE_SOURCES_COMMON)
testplugin_x86_linux_CPPFLAGS     = $(AM_CPPFLAGS_X86_LINUX)
testplugin_x86_linux_CFLAGS       = $(AM_CFLAGS_X86_LINUX)
testplugin_x86_linux_DEPENDENCIES = $(COREGRIND_LIBS_X86_LINUX)
testplugin_x86_linux_LDADD        = $(TOOL_LDADD_X86_LINUX)
testplugin_x86_linux_LDFLAGS      = $(TOOL_LDFLAGS_X86_LINUX)

testplugin_amd64_linux_SOURCES      = $(NONE_SOURCES_COMMON)
testplugin_amd64_linux_CPPFLAGS     = $(AM_CPPFLAGS_AMD64_LINUX)
testplugin_amd64_linux_CFLAGS       = $(AM_CFLAGS_AMD64_LINUX)
testplugin_amd64_linux_DEPENDENCIES = $(COREGRIND_LIBS_AMD64_LINUX)
testplugin_amd64_linux_LDADD        = $(TOOL_LDADD_AMD64_LINUX)
testplugin_amd64_linux_LDFLAGS      = $(TOOL_LDFLAGS_AMD64_LINUX)

testplugin_ppc32_linux_SOURCES      = $(NONE_SOURCES_COMMON)
testplugin_ppc32_linux_CPPFLAGS     = $(AM_CPPFLAGS_PPC32_LINUX)
testplugin_ppc32_linux_CFLAGS       = $(AM_CFLAGS_PPC32_LINUX)
testplugin_ppc32_linux_DEPENDENCIES = $(COREGRIND_LIBS_PPC32_LINUX)
testplugin_ppc32_linux_LDADD        = $(TOOL_LDADD_PPC32_LINUX)
testplugin_ppc32_linux_LDFLAGS      = $(TOOL_LDFLAGS_PPC32_LINUX)

testplugin_ppc64_linux_SOURCES      = $(NONE_SOURCES_COMMON)
testplugin_ppc64_linux_CPPFLAGS     = $(AM_CPPFLAGS_PPC64_LINUX)
testplugin_ppc64_linux_CFLAGS       = $(AM_CFLAGS_PPC64_LINUX)
testplugin_ppc64_linux_DEPENDENCIES = $(COREGRIND_LIBS_PPC64_LINUX)
testplugin_ppc64_linux_LDADD        = $(TOOL_LDADD_PPC64_LINUX)
testplugin_ppc64_linux_LDFLAGS      = $(TOOL_LDFLAGS_PPC64_LINUX)

testplugin_ppc32_aix5_SOURCES      = $(NONE_SOURCES_COMMON)
testplugin_ppc32_aix5_CPPFLAGS     = $(AM_CPPFLAGS_PPC32_AIX5)
testplugin_ppc32_aix5_CFLAGS       = $(AM_CFLAGS_PPC32_AIX5)
testplugin_ppc32_aix5_DEPENDENCIES = $(COREGRIND_LIBS_PPC32_AIX5)
testplugin_ppc32_aix5_LDADD        = $(TOOL_LDADD_PPC32_AIX5)
testplugin_ppc32_aix5_LDFLAGS      = $(TOOL_LDFLAGS_PPC32_AIX5)

testplugin_ppc64_aix5_SOURCES      = $(NONE_SOURCES_COMMON)
testplugin_ppc64_aix5_CPPFLAGS     = $(AM_CPPFLAGS_PPC64_AIX5)
testplugin_ppc64_aix5_CFLAGS       = $(AM_CFLAGS_PPC64_AIX5)
testplugin_ppc64_aix5_DEPENDENCIES = $(COREGRIND_LIBS_PPC64_AIX5)
testplugin_ppc64_aix5_LDADD        = $(TOOL_LDADD_PPC64_AIX5)
testplugin_ppc64_aix5_LDFLAGS      = $(TOOL_LDFLAGS_PPC64_AIX5)

noinst_headers = mem_tracer.h
