# Makefile for libzeep and a test/sample application using libzeep.
#
#  Copyright Maarten L. Hekkelman, Radboud University 2008-2010.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# You may have to edit the first three defines on top of this
# makefile to match your current installation.

firstTarget: all

# installation prefix
PREFIX              ?= /usr/local

# main build variables
CXX                 ?= c++
CXXFLAGS            += $(BOOST_INC_DIR:%=-I%) -I. -pthread -std=c++14
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

-include ../make.config

BOOST_LIB_DIR       = $(BOOST:%=%/lib)
BOOST_INC_DIR       = $(BOOST:%=%/include)

BOOST_LIBS          = system thread filesystem regex random program_options
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

PROGRAMS			= parser-test xpath-test json-test processor-test

OBJECTS = $(PROGRAMS:%=$(OBJDIR)/%.o)
	
-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo ">>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

all: $(PROGRAMS)
.PHONY: all

../lib/libzeep.a: FORCE
	+$(MAKE) -C ../lib DEBUG=$(DEBUG) static-lib

$(PROGRAMS): ../lib/libzeep.a

$(PROGRAMS): %: $(OBJDIR)/%.o
	@ echo '->' $@
	@ $(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) libzeep.a libzeep.so XML-Test-Suite/ $(PROGRAMS)

dist-clean: clean

FORCE:
