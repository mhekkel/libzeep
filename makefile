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

# main build variables
CXX                 ?= c++
CXXFLAGS            += -O2 $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -std=c++14
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LDFLAGS				= -g

# default is to only create a static library
BUILD_STATIC_LIB	= 1
BUILD_SHARED_LIB	= 0

# Use the DEBUG flag to build debug versions of the code
DEBUG               = 0

-include make.config

make.config:
	@echo "# Use this file to override some of the predefined variables" > $@
	@echo "#" >> $@
	@echo "# Specifically, you might want to change these:" >> $@
	@echo "# PREFIX = $(PREFIX)" >> $@
	@echo "# CXX = $(CXX)" >> $@
	@echo "# CXXFLAGS = $(CXXFLAGS)" >> $@
	@echo "# LD = $(LD)" >> $@
	@echo "# LDFLAGS = $(LDFLAGS)" >> $@
	@echo "# BOOST = $(HOME)/my-boost" >> $@
	@echo "# BOOST_LIB_SUFFIX = $(BOOST_LIB_SUFFIX)" >> $@
	@echo "#" >> $@
	@echo "# Default is to build only static libraries, you can change that here" >> $@
	@echo "# BUILD_STATIC_LIB = $(BUILD_STATIC_LIB)" >> $@
	@echo "# BUILD_SHARED_LIB = $(BUILD_SHARED_LIB)" >> $@
	@echo "#" >> $@
	@echo "# To build a debug version of the library set DEBUG to 1" >> $@
	@echo "# DEBUG = $(DEBUG)" >> $@
	echo "Wrote a new make.config file"

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

# targets

ifeq "$(BUILD_STATIC_LIB)" "1"
LIB_TARGETS	+= static-lib
endif

ifeq "$(BUILD_SHARED_LIB)" "1"
LIB_TARGETS	+= shared-lib
endif

.PHONY: libs
libs:
	+$(MAKE) -C lib $(LIB_TARGETS)

.PHONY: test
test: libs
	+$(MAKE) -C test all-tests

.PHONY: examples
examples: libs
	+$(MAKE) -C examples all

.PHONY: doc
doc:
	cd doc; bjam

.PHONY: all	
all: libs test examples

.PHONY: clean
clean:
	$(MAKE) -C lib clean
	$(MAKE) -C test clean
	$(MAKE) -C examples clean
	cd doc; bjam clean
	rm -f $(DIST_NAME).tgz

.PHONY: dist-clean
dist-clean:
	$(MAKE) -C lib dist-clean
	$(MAKE) -C test dist-clean
	$(MAKE) -C examples dist-clean
	rm -f $(DIST_NAME).tgz

.PHONY: dist
dist: doc
	rm -rf $(DIST_NAME)
	svn export . $(DIST_NAME)
	find doc/html -depth | cpio -pd $(DIST_NAME)
	tar czf $(DIST_NAME).tgz $(DIST_NAME)
	rm -rf $(DIST_NAME)
