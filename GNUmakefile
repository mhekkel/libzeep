# Master makefile for libzeep
#
#          copyright 2019, Maarten L. Hekkelman
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# This makefile can use a local make.config file to
# override some of the default settings.
# 

firstTarget: all

# installation prefix
PREFIX              ?= /usr/local

-include make.config

# main build variables
CXX                 ?= c++
CXXFLAGS            += -O2 -I. -fPIC -pthread -std=c++17
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g
BJAM				?= b2

# default is to only create a static library
BUILD_STATIC_LIB	?= 1
BUILD_SHARED_LIB	?= 0

# Use the DEBUG flag to build debug versions of the code
DEBUG               ?= 0

make.config:
	@ echo "# Use this file to override some of the predefined variables" > $@
	@ echo "#" >> $@
	@ echo "# Specifically, you might want to change these:" >> $@
	@ echo "# PREFIX = $(PREFIX)" >> $@
	@ echo "# CXX = $(CXX)" >> $@
	@ echo "# CXXFLAGS = $(CXXFLAGS)" >> $@
	@ echo "# LD = $(LD)" >> $@
	@ echo "# LDFLAGS = $(LDFLAGS)" >> $@
	@ echo "# BOOST = $(HOME)/my-boost" >> $@
	@ echo "# BOOST_LIB_SUFFIX = $(BOOST_LIB_SUFFIX)" >> $@
	@ echo "#" >> $@
	@ echo "# Default is to build only static libraries, you can change that here" >> $@
	@ echo "# BUILD_STATIC_LIB = $(BUILD_STATIC_LIB)" >> $@
	@ echo "# BUILD_SHARED_LIB = $(BUILD_SHARED_LIB)" >> $@
	@ echo "#" >> $@
	@ echo "# To build a debug version of the library set DEBUG to 1" >> $@
	@ echo "# (or better, specify it on the command line like so: make DEBUG=1")" >> $@
	@ echo "# DEBUG = $(DEBUG)" >> $@
	@ echo "Wrote a new make.config file"

VERSION_MAJOR       = 5.0
VERSION_MINOR       = 0
VERSION             = $(VERSION_MAJOR).$(VERSION_MINOR)
DIST_NAME           = libzeep-$(VERSION)

BOOST_LIB_DIR       = $(BOOST:%=%/lib)
BOOST_INC_DIR       = $(BOOST:%=%/include)

OUTPUT_DIR			?= ./lib

LIBDIR              ?= $(PREFIX)/lib
INCDIR              ?= $(PREFIX)/include
MANDIR              ?= $(PREFIX)/man/man3
DOCDIR              ?= $(PREFIX)/share/doc/libzeep-doc

# targets

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

ZEEP_LIB_PARTS		= xml json http
ZEEP_SHARED_LIBS	+= $(ZEEP_LIB_PARTS:%=$(OUTPUT_DIR)/libzeep_%.so)
ZEEP_STATIC_LIBS	+= $(ZEEP_LIB_PARTS:%=$(OUTPUT_DIR)/libzeep_%.a)

ifneq ($(BUILD_SHARED_LIB),0)
ZEEP_LIBS += $(ZEEP_SHARED_LIBS)
endif

ifneq ($(BUILD_STATIC_LIB),0)
ZEEP_LIBS += $(ZEEP_STATIC_LIBS)
endif

define ZEEPLIB_template = 

 .PHONY: $(1)_clean
 $(1)_clean:
	+$(MAKE) -C lib-$(1) clean

 .PHONY: $(1)_test
 $(1)_test: static-libs
	+$(MAKE) -C lib-$(1) test

 $(OUTPUT_DIR)/libzeep_$(1).a: FORCE | $(OUTPUT_DIR)
	+$(MAKE) -C lib-$(1) static-lib

 $(OUTPUT_DIR)/libzeep_$(1).so: FORCE | $(OUTPUT_DIR)
	+$(MAKE) -C lib-$(1) shared-lib
endef

$(foreach part,$(ZEEP_LIB_PARTS),$(eval $(call ZEEPLIB_template,$(part))))

.PHONY: libraries
libraries: $(ZEEP_LIBS)

.PHONY: static-libs
static-libs: $(ZEEP_STATIC_LIBS)

.PHONY: shared-libs
shared-libs: $(ZEEP_SHARED_LIBS)

.PHONY: test
test: $(ZEEP_LIB_PARTS:%=%_test)

.PHONY: examples
examples: libraries
	+$(MAKE) -C examples all

.PHONY: doc
doc:
	cd doc; $(BJAM)

.PHONY: all	
all: libraries test examples

.PHONY: clean
clean: $(ZEEP_LIB_PARTS:%=%_clean)
	$(MAKE) -C examples clean
	cd doc; $$(which $(BJAM) > /dev/null) && $(BJAM) clean || echo "No $(BJAM) installed, cannot clean doc"
	rm -f $(DIST_NAME).tgz

.PHONY: dist
dist: doc
	rm -rf $(DIST_NAME)
	svn export . $(DIST_NAME)
	find doc/html -depth | cpio -pd $(DIST_NAME)
	tar czf $(DIST_NAME).tgz $(DIST_NAME)
	rm -rf $(DIST_NAME)

.PHONY: FORCE
FORCE: