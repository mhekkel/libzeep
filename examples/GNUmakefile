# Makefile for the libzeep-http library
#
# Copyright Maarten L. Hekkelman, UMC St. Radboud 2008-2013.
#        Copyright Maarten L. Hekkelman, 2014-2019
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# This makefile assumes you already installed libzeep

.PHONY: firstTarget
firstTarget: all

# main build variables
CXXFLAGS            += -pthread -std=c++17
WARNINGS            += all no-multichar

ifeq "$(DEBUG)" "1"
DEFINES				+= DEBUG
CXXFLAGS            += -g -O0
LDFLAGS				+= -g
else
CXXFLAGS			+= -O3
DEFINES				+= NDEBUG
endif

CXXFLAGS			+= $(DEFINES:%=-D%)
CXXFLAGS			+= $(WARNINGS:%=-W%)

OBJDIR = obj
ifeq "$(DEBUG)" "1"
	OBJDIR	:= $(OBJDIR).dbg
endif

BINDIR = bin

$(OBJDIR) $(BINDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo "cc>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

.PHONY: clean
clean:
	rm -rf $(OBJDIR)/* $(EXAMPLES:%=$(BINDIR)/%)

ZEEP_LIBS = http xml json
BOOST_LIBS = system

define EXAMPLE_template =

-include $$(OBJDIR)/$(1).d

$(1)_OBJECTS = $$(OBJDIR)/$(1).o

$(BINDIR)/$(1): $$($(1)_OBJECTS) | $(BINDIR)
	@ echo "ld> $(1)"
	@ $(CXX) -o $$@ $$($(1)_OBJECTS) $$(CFLAGS) $$(CXXFLAGS) $(LDFLAGS) $(ZEEP_LIBS:%=-lzeep-%) $(BOOST_LIBS:%=-lboost_%) $(LIBS)

endef

EXAMPLES = \
	http-server-0 http-server-1 http-server-2 \
	serialize-xml synopsis-json synopsis-xml validating-xml-sample xpath-sample \
	synopsis-el-1 security-sample rest-sample rest-sample-2

$(foreach part,$(EXAMPLES),$(eval $(call EXAMPLE_template,$(part))))

.PHONY: examples
examples: $(EXAMPLES:%=$(BINDIR)/%)

all: examples

FORCE:

