# nbdkit
# Copyright Red Hat
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# * Neither the name of Red Hat nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY RED HAT AND CONTRIBUTORS ''AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RED HAT OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

include $(top_srcdir)/common-rules.mk

EXTRA_DIST = nbdkit-partitioning-plugin.pod

# Disabled on Windows because device_size is not supported.
if !IS_WINDOWS

plugin_LTLIBRARIES = nbdkit-partitioning-plugin.la

nbdkit_partitioning_plugin_la_SOURCES = \
	partitioning.c \
	partition-gpt.c \
	partition-mbr.c \
	virtual-disk.c \
	virtual-disk.h \
	$(top_srcdir)/include/nbdkit-plugin.h \
	$(NULL)

nbdkit_partitioning_plugin_la_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/common/gpt \
	-I$(top_srcdir)/common/include \
	-I$(top_srcdir)/common/regions \
	-I$(top_srcdir)/common/replacements \
	-I$(top_srcdir)/common/utils \
	-I. \
	$(NULL)
nbdkit_partitioning_plugin_la_CFLAGS = $(WARNINGS_CFLAGS)
nbdkit_partitioning_plugin_la_LDFLAGS = \
	-module -avoid-version -shared $(NO_UNDEFINED_ON_WINDOWS) \
	$(NULL)
if USE_LINKER_SCRIPT
nbdkit_partitioning_plugin_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/plugins/plugins.syms
endif
nbdkit_partitioning_plugin_la_LIBADD = \
	$(top_builddir)/common/gpt/libgpt.la \
	$(top_builddir)/common/regions/libregions.la \
	$(top_builddir)/common/utils/libutils.la \
	$(top_builddir)/common/replacements/libcompat.la \
	$(IMPORT_LIBRARY_ON_WINDOWS) \
	$(NULL)

if HAVE_POD

man_MANS = nbdkit-partitioning-plugin.1
CLEANFILES += $(man_MANS)

nbdkit-partitioning-plugin.1: nbdkit-partitioning-plugin.pod \
		$(top_builddir)/podwrapper.pl
	$(PODWRAPPER) --section=1 --man $@ \
	    --html $(top_builddir)/html/$@.html \
	    $<

endif HAVE_POD
endif !IS_WINDOWS
