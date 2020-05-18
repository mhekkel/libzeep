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

PACKAGES			+=

# default is to only create a static library
BUILD_STATIC_LIB	= 1
BUILD_SHARED_LIB	= 0

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

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

-include ../make.config

ifneq ($(PACKAGES),)
CXXFLAGS			+= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PACKAGES))
LDFLAGS				+= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PACKAGES) --static )
endif

VERSION_MAJOR       = 4.0
VERSION_MINOR       = 0
VERSION             = $(VERSION_MAJOR).$(VERSION_MINOR)

LIB_NAME			= libzeep-html

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

CFLAGS				+= $(DEFINES:%=-D%)
CXXFLAGS			+= $(DEFINES:%=-D%)

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
	$(OBJDIR)/authorization.o \
	$(OBJDIR)/el-processing.o \
	$(OBJDIR)/tag-processor.o \
	$(OBJDIR)/tag-processor-v2.o \
	$(OBJDIR)/controller.o

ifneq ($(USE_RSRC),0)
OBJECTS += \
	$(OBJDIR)/controller-rsrc.o \
	$(OBJDIR)/test_rsrc.o
endif

HEADERS = \
	zeep/http/md5.hpp \
	zeep/http/tag-processor.hpp \
	zeep/html/controller.hpp

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
	install -d $(LIBDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http $(INCDIR)/zeep/html/controller
	for f in $(HEADERS); do install ../include/$$f $(INCDIR)/$$f; done
	install $(STATIC_LIB) $(LIBDIR)/$(STATIC_LIB_NAME)
	strip -SX $(LIBDIR)/$(STATIC_LIB_NAME)

install: install-libs install-dev

src/mrsrc.h:
	$(MRC) --header > $@

-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@ echo ">>" $<
	@ $(CXX) -MD -c -o $@ $< $(CFLAGS) $(CXXFLAGS)

clean:
	rm -rf $(OBJDIR)/* $(OUTPUT)/$(LIB_NAME)*

dist-clean: clean

# $(OBJDIR)/unit-test.d: $(OBJDIR)/unit-test.o

$(OBJDIR)/test_rsrc.o: test/fragment-file.xhtml src/mrsrc.h
	mrc -o $@ $<

webapp_PARAMS = 

ZEEP_LIBS = http xml json generic
BOOST_LIBS = system

define TEST_template =

-include $$(OBJDIR)/$(1)-test.d

$(1)_OBJECTS = $$(OBJECTS) $$(OBJDIR)/$(1)-test.o

test/$(1)-test: $$(OBJECTS) $$($(1)_OBJECTS)
	@ echo ">>> building $(1)-test"
	@ $(CXX) -o $$@ $$(OBJDIR)/$(1)-test.o $$(OBJECTS) $$(CFLAGS) $$(CXXFLAGS) $(LDFLAGS) -L../lib $(ZEEP_LIBS:%=-lzeep-%) $(BOOST_LIBS:%=-lboost_%) -lstdc++fs

.PHONY: $(1)-test
$(1)-test: test/$(1)-test
	cd test; ./$(1)-test $$($(1)_PARAMS)

endef

TESTS = webapp processor

ifneq ($(USE_RSRC),0)
TESTS += rsrc_webapp
endif

$(foreach part,$(TESTS),$(eval $(call TEST_template,$(part))))

.PHONY: test
test: $(TESTS:%=%-test)

FORCE:

