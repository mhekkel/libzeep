# Makefile for the libzeep-http library
#
# Copyright Maarten L. Hekkelman, UMC St. Radboud 2008-2013.
#        Copyright Maarten L. Hekkelman, 2014-2019
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# Use the make.config file in the uplevel directory to
# change the settings for this build

firstTarget: all

# main build variables
CXX                 ?= c++
CFLAGS	            += $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -std=c++17 -Wall -g
LD                  ?= ld
LDFLAGS				= -g

# Use the DEBUG flag to build debug versions of the code
DEBUG               ?= 0

MRC					?= mrc

# Using resources should be the default
ifeq ($(USE_RSRC),)
ifeq ($(shell which $(MRC)),)
$(warning "")
$(warning "The executable mrc is not found in your path.")
$(warning "Please consider installing it to enable compiled-in data resources.")
$(warning "See https://github.com/mhekkel/mrc")
$(warning "")
	USE_RSRC = 0
else
	USE_RSRC = 1
endif
endif

ifneq ($(USE_RSRC),0)
	DEFINES		+= WEBAPP_USES_RESOURCES
endif

# include the global configuration file
include ../make.config

BOOST_INC_DIR		?= $(BOOST)/include
BOOST_LIB_DIR		?= $(BOOST)/lib

CFLAGS				+= -I $(BOOST_INC_DIR)
LDFLAGS				+= -L $(BOOST_LIB_DIR)

CFLAGS				+= $(DEFINES:%=-D%)

ifeq "$(DEBUG)" "1"
CFLAGS				+= -DDEBUG
else
CFLAGS				+= -O2
endif

# targets

CFLAGS				+= -I ../../include

OBJDIR = obj
ifeq "$(DEBUG)" "1"
	OBJDIR	:= $(OBJDIR).dbg
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo "cc>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

$(OBJDIR)/dummy_rsrc.o:
	$(MRC) -o $@ test.xml

clean:
	rm -rf $(OBJDIR)/* $(EXAMPLES)

ZEEP_LIBS = http xml json
BOOST_LIBS = system

define EXAMPLE_template =

-include $$(OBJDIR)/$(1).d

$(1)_OBJECTS = $$(OBJDIR)/$(1).o

ifneq ($$(USE_RSRC),0)
$(1)_OBJECTS += $$(OBJDIR)/dummy_rsrc.o
endif

$(1): $$($(1)_OBJECTS)
	@ echo "ld> $(1)"
	@ $(CXX) -o $$@ $$($(1)_OBJECTS) $$(CFLAGS) $$(CXXFLAGS) $(LDFLAGS) -L../../lib $(ZEEP_LIBS:%=-lzeep-%) $(BOOST_LIBS:%=-lboost_%) -lstdc++fs

endef

EXAMPLES = \
	http-server-0 http-server-1 http-server-2 \
	serialize-xml synopsis-json synopsis-xml validating-xml-sample xpath-sample \
	synopsis-el-1 security-sample rest-sample

$(foreach part,$(EXAMPLES),$(eval $(call EXAMPLE_template,$(part))))

.PHONY: examples
examples: $(EXAMPLES)

all: examples

FORCE:

