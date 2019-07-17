# Makefile for libzeep, the library
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

# main build variables
CXX                 ?= c++
CXXFLAGS            += $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -std=c++14
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g

# default is to only create a static library
BUILD_STATIC_LIB	= 1
BUILD_SHARED_LIB	= 0

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

-include ../make.config

VERSION_MAJOR       = 4.0
VERSION_MINOR       = 0
VERSION             = $(VERSION_MAJOR).$(VERSION_MINOR)
DIST_NAME           = libzeep-$(VERSION)
SO_NAME             = libzeep.so.$(VERSION_MAJOR)
LIB_NAME            = $(SO_NAME).$(VERSION_MINOR)

BOOST_LIB_DIR       = $(BOOST:%=%/lib)
BOOST_INC_DIR       = $(BOOST:%=%/include)

LIBDIR              ?= $(PREFIX)/lib
INCDIR              ?= $(PREFIX)/include
MANDIR              ?= $(PREFIX)/man/man3
DOCDIR              ?= $(PREFIX)/share/doc/libzeep-doc

BOOST_LIBS          = system thread filesystem regex random
BOOST_LIBS          := $(BOOST_LIBS:%=boost_%$(BOOST_LIB_SUFFIX))
LIBS                = $(BOOST_LIBS) stdc++ m pthread
LDFLAGS             += $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%) -g

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

OBJECTS = \
	$(OBJDIR)/connection.o \
	$(OBJDIR)/controller.o \
	$(OBJDIR)/doctype.o \
	$(OBJDIR)/document.o \
	$(OBJDIR)/element.o \
	$(OBJDIR)/exception.o \
	$(OBJDIR)/http-server.o \
	$(OBJDIR)/json-parser.o \
	$(OBJDIR)/md5.o \
	$(OBJDIR)/message_parser.o \
	$(OBJDIR)/node.o \
	$(OBJDIR)/parser-internals.o \
	$(OBJDIR)/preforked-http-server.o \
	$(OBJDIR)/reply.o \
	$(OBJDIR)/request.o \
	$(OBJDIR)/rest-controller.o \
	$(OBJDIR)/soap-envelope.o \
	$(OBJDIR)/soap-server.o \
	$(OBJDIR)/tag-processor.o \
	$(OBJDIR)/tag-processor-v2.o \
	$(OBJDIR)/unicode_support.o \
	$(OBJDIR)/webapp.o \
	$(OBJDIR)/webapp-el.o \
	$(OBJDIR)/writer.o \
	$(OBJDIR)/xml-parser.o \
	$(OBJDIR)/xpath.o

libzeep.a: $(OBJECTS)
	ar rc $@ $(OBJECTS)
	ranlib $@

$(LIB_NAME): $(OBJECTS)
	$(CXX) -shared -o $@ -Wl,-soname=$(SO_NAME) $(OBJECTS) $(LDFLAGS)

$(SO_NAME): $(LIB_NAME)
	ln -fs $(LIB_NAME) $@

libzeep.so: $(SO_NAME)
	ln -fs $(LIB_NAME) $@

ifeq "$(BUILD_STATIC_LIB)" "1"
libs: libzeep.a
endif

ifeq "$(BUILD_SHARED_LIB)" "1"
libs: libzeep.so
endif
.PHONY: libs

static-lib: libzeep.a
.PHONY: static-lib

shared-lib: libzeep.so
.PHONY: shared-lib

install-libs: libs
	install -d $(LIBDIR)
	install $(LIB_NAME) $(LIBDIR)/$(LIB_NAME)
	strip --strip-unneeded $(LIBDIR)/$(LIB_NAME)
	ln -Tfs $(LIB_NAME) $(LIBDIR)/$(SO_NAME)
	ln -Tfs $(LIB_NAME) $(LIBDIR)/libzeep.so
	$(LD_CONFIG) -n $(LIBDIR)

install-dev: libzeep.a
	install -d $(LIBDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http $(INCDIR)/zeep/http/webapp
	for f in `find zeep -name "*.hpp"`; do install $$f $(INCDIR)/$$f; done
	install ./libzeep.a $(LIBDIR)/libzeep.a
	strip -SX $(LIBDIR)/libzeep.a

install: install-libs install-dev

-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo ">>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

clean:
	rm -rf $(OBJDIR)/* libzeep.a libzeep.so*

dist-clean: clean

FORCE:
