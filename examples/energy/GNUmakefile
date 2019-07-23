# Makefile for example app using libzeep
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

# installation prefix
PREFIX              ?= /usr/local

PACKAGES			+= libpqxx

# main build variables
CXX                 ?= clangc++
CXXFLAGS            += $(BOOST_INC_DIR:%=-I%) -I. -pthread -std=c++17
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

-include make.config

ifneq ($(PACKAGES),)
CXXFLAGS			+= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PACKAGES))
LDFLAGS				+= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PACKAGES) --static )
endif

ZEEP_INC_DIR		= ../../include
ZEEP_LIB_DIR		= ../../lib

BOOST_INC_DIR       = $(BOOST:%=%/include)
BOOST_LIB_DIR       = $(BOOST:%=%/lib)

BOOST_LIBS          = system thread filesystem regex random program_options date_time
BOOST_LIBS          := $(BOOST_LIBS:%=boost_%$(BOOST_LIB_SUFFIX))
LIBS                := zeep $(BOOST_LIBS) stdc++ m pthread $(LIBS)
LDFLAGS             += $(BOOST_LIB_DIR:%=-L%) -L../lib $(LIBS:%=-l%) -g

ifneq "$(DEBUG)" "1"
CXXFLAGS			+= -O2
endif

# targets

VPATH += src

CXXFLAGS			+= $(ZEEP_INC_DIR:%=-I%) $(BOOST_INC_DIR:%=-I%)
LDFLAGS				+= $(ZEEP_LIB_DIR:%=-L%)

OBJDIR = obj
ifeq "$(DEBUG)" "1"
	OBJDIR	:= $(OBJDIR).dbg
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

APPS = energyd

OBJECTS = $(APPS:%=$(OBJDIR)/%.o)
	
-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo ">>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

all: energyd
.PHONY: all

docroot/scripts:
	mkdir -p $@

docroot/scripts/grafiek.js docroot/scripts/opname.js: webapp/grafiek.js webapp/opname.js | docroot/scripts
	yarn webpack

assets: docroot/scripts/grafiek.js docroot/scripts/opname.js | docroot/scripts

.PHONY: assets

$(ZEEP_LIB_DIR)/libzeep.a: FORCE
	+$(MAKE) -C $(ZEEP_LIB_DIR) static-lib

energyd: $(ZEEP_LIB_DIR)/libzeep.a

energyd: %: $(OBJDIR)/%.o | assets
	@ echo '->' $@
	@ $(CXX) -o $@ $^ $(LDFLAGS)
	@ $(MAKE) assets

clean:
	rm -rf $(OBJDIR)/* energyd

dist-clean: clean

FORCE:
