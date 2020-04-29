# Makefile for the libzeep-generic library
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
CXXFLAGS            += $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -std=c++17
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g
LD_CONFIG			?= ldconfig

# default is to only create a static library
BUILD_STATIC_LIB	= 1
BUILD_SHARED_LIB	= 0

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

-include ../make.config

VERSION_MAJOR       = 4.0
VERSION_MINOR       = 0
VERSION             = $(VERSION_MAJOR).$(VERSION_MINOR)

LIB_NAME			= libzeep-soap

DIST_NAME           = $(LIB_NAME)$(VERSION)
SO_NAME             = $(LIB_NAME).so.$(VERSION_MAJOR)
SHARED_LIB_NAME     = $(SO_NAME).$(VERSION_MINOR)
STATIC_LIB_NAME		= $(LIB_NAME).a

OUTPUT				= ../lib/

SHARED_LIB			= $(OUTPUT)/$(SHARED_LIB_NAME)
STATIC_LIB			= $(OUTPUT)/$(STATIC_LIB_NAME)

LIBDIR              ?= $(PREFIX)/lib
INCDIR              ?= $(PREFIX)/include
MANDIR              ?= $(PREFIX)/man/man3
DOCDIR              ?= $(PREFIX)/share/doc/libzeep-doc

BOOST_INC_DIR		?= $(BOOST)/include
BOOST_LIB_DIR		?= $(BOOST)/lib

CFLAGS				+= -I $(BOOST_INC_DIR)
CXXFLAGS			+= -I $(BOOST_INC_DIR)
LDFLAGS				+= -L $(BOOST_LIB_DIR)

ifeq "$(DEBUG)" "1"
CXXFLAGS			+= -DDEBUG
else
CXXFLAGS			+= -O2
endif

# targets

VPATH += src:test

CXXFLAGS += -I ../include

OBJDIR = obj
ifeq "$(DEBUG)" "1"
	OBJDIR	:= $(OBJDIR).dbg
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTPUT):
	mkdir -p $(OUTPUT)

OBJECTS = \
	$(OBJDIR)/envelope.o \
	$(OBJDIR)/controller.o 

HEADERS = \
	zeep/http/controller.hpp \
	zeep/http/envelope.hpp

$(STATIC_LIB): $(OBJECTS)
	ar rc $@ $(OBJECTS)
	ranlib $@

$(SHARED_LIB): $(OBJECTS)
	$(CXX) -shared -o $(SHARED_LIB) -Wl,-soname=$(SO_NAME) $(OBJECTS) $(LDFLAGS)
	ln -fs $(SHARED_LIB_NAME) $(OUTPUT)/$(SO_NAME)
	ln -fs $(SHARED_LIB_NAME) $(OUTPUT)/$(LIB_NAME).so

ifeq "$(BUILD_STATIC_LIB)" "1"
BUILD_LIBS = $(STATIC_LIB)
endif

ifeq "$(BUILD_SHARED_LIB)" "1"
BUILD_LIBS += $(SHARED_LIB)
endif

libs: $(BUILD_LIBS)
.PHONY: libs

static-lib: $(STATIC_LIB)
.PHONY: static-lib

shared-lib: $(SHARED_LIB)
.PHONY: shared-lib

install-libs: $(SHARED_LIB)
	install -d $(LIBDIR)
	install $(SHARED_LIB) $(LIBDIR)/$(SHARED_LIB_NAME)
	strip --strip-unneeded $(LIBDIR)/$(SHARED_LIB_NAME)
	ln -Tfs $(SHARED_LIB_NAME) $(LIBDIR)/$(SO_NAME)
	ln -Tfs $(SHARED_LIB_NAME) $(LIBDIR)/$(LIB_NAME).so
	$(LD_CONFIG) -n $(LIBDIR)

install-dev: $(STATIC_LIB)
	install -d $(LIBDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http $(INCDIR)/zeep/http/webapp
	for f in $(HEADERS); do install ../include/$$f $(INCDIR)/$$f; done
	install $(STATIC_LIB) $(LIBDIR)/$(STATIC_LIB_NAME)
	strip -SX $(LIBDIR)/$(STATIC_LIB_NAME)

install: install-libs install-dev

-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo ">>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

clean:
	rm -rf $(OBJDIR)/* $(OUTPUT)/$(LIB_NAME)*

dist-clean: clean

-include $(OBJDIR)/soap-test.d

# $(OBJDIR)/unit-test.d: $(OBJDIR)/unit-test.o

soap_PARAMS = 

define TEST_template =

test/$(1)-test: $$(OBJECTS) $$(OBJDIR)/$(1)-test.o
	@ echo ">>> building $(1)-test"
	@ $(CXX) -o $$@ $$(OBJDIR)/$(1)-test.o $$(OBJECTS) $$(CFLAGS) $$(CXXFLAGS) -L../lib -lzeep-xml -lzeep-http -lzeep-el -lzeep-generic -lboost_program_options -lboost_filesystem -lboost_system

.PHONY: $(1)-test
$(1)-test: test/$(1)-test
	test/$(1)-test $$($(1)_PARAMS)

endef

TESTS = soap
$(foreach part,$(TESTS),$(eval $(call TEST_template,$(part))))

.PHONY: test
test: $(TESTS:%=%-test)

FORCE:

