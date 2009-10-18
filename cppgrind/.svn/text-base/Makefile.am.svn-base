STL_FLAGS = -I$(top_srcdir)/include/m_stl # libstdc++/gcc
#STL_FLAGS = -I$(top_srcdir)/include/stlport -DUSE_STLPORT # STLPORT
# don't forget to make clean before re-building the tool with different STL impl

include $(top_srcdir)/Makefile.tool.am

noinst_PROGRAMS = 
if VGP_X86_LINUX
noinst_PROGRAMS += cppgrind-x86-linux
endif
if VGP_AMD64_LINUX
noinst_PROGRAMS += cppgrind-amd64-linux
endif

cppgrind_SOURCES_COMMON = cp_main.cc

#TODO more platforms here?
cppgrind_x86_linux_SOURCES      = $(cppgrind_SOURCES_COMMON)
cppgrind_x86_linux_CPPFLAGS     = $(AM_CPPFLAGS_X86_LINUX)
cppgrind_x86_linux_CXXFLAGS     = $(AM_CXXFLAGS_X86_LINUX)
cppgrind_x86_linux_CFLAGS       = $(AM_CFLAGS_X86_LINUX)
cppgrind_x86_linux_DEPENDENCIES = $(COREGRIND_LIBS_X86_LINUX)
cppgrind_x86_linux_LDADD        = $(TOOL_LDADD_X86_LINUX)
cppgrind_x86_linux_LDFLAGS      = $(TOOL_LDFLAGS_X86_LINUX)

cppgrind_amd64_linux_SOURCES      = $(cppgrind_SOURCES_COMMON)
cppgrind_amd64_linux_CPPFLAGS     = $(AM_CPPFLAGS_AMD64_LINUX)
cppgrind_amd64_linux_CXXFLAGS     = $(AM_CXXFLAGS_AMD64_LINUX)
cppgrind_amd64_linux_CFLAGS       = $(AM_CFLAGS_AMD64_LINUX)
cppgrind_amd64_linux_DEPENDENCIES = $(COREGRIND_LIBS_AMD64_LINUX)
cppgrind_amd64_linux_LDADD        = $(TOOL_LDADD_AMD64_LINUX)
cppgrind_amd64_linux_LDFLAGS      = $(TOOL_LDFLAGS_AMD64_LINUX)
