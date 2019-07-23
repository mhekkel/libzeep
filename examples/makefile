# Makefile for libzeep, the examples
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
CXX                 ?= c++
CXXFLAGS            += $(BOOST_INC_DIR:%=-I%) -I. -pthread -std=c++17
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

-include ../make.config

ifneq ($(PACKAGES),)
CXXFLAGS			+= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PACKAGES))
LDFLAGS				+= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PACKAGES) --static )
endif

BOOST_LIB_DIR       = $(BOOST:%=%/lib)
BOOST_INC_DIR       = $(BOOST:%=%/include)

BOOST_LIBS          = system thread filesystem regex random program_options date_time
BOOST_LIBS          := $(BOOST_LIBS:%=boost_%$(BOOST_LIB_SUFFIX))
LIBS                = zeep $(BOOST_LIBS) stdc++ m pthread
LDFLAGS             += $(BOOST_LIB_DIR:%=-L%) -L../lib $(LIBS:%=-l%) -g

ifneq "$(DEBUG)" "1"
CXXFLAGS			+= -O2
endif

# targets

VPATH += src

CXXFLAGS += -I ../include

OBJDIR = obj
ifeq "$(DEBUG)" "1"
	OBJDIR	:= $(OBJDIR).dbg
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

TEST_APPS = rest-test zeep-test webapp-test

OBJECTS = $(TEST_APPS:%=$(OBJDIR)/%.o)
	
-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo ">>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

all: $(TEST_APPS)
.PHONY: all

../lib/libzeep.a:
	+$(MAKE) -C ../lib static-lib

$(TEST_APPS): ../lib/libzeep.a

$(TEST_APPS): %: $(OBJDIR)/%.o
	@ echo '->' $@
	@ $(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR)/* $(TEST_APPS)

dist-clean: clean

FORCE:
