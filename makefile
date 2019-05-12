# Makefile for libzeep and a test/sample application using libzeep.
#
# Copyright Maarten L. Hekkelman, UMC St. Radboud 2008-2013.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# You may have to edit the first three defines on top of this
# makefile to match your current installation.

#BOOST_LIB_SUFFIX   = # e.g. '-mt'
#BOOST              = $(HOME)/projects/boost
BOOST_LIB_DIR       = $(BOOST:%=%/lib)
BOOST_INC_DIR       = $(BOOST:%=%/include)

PREFIX              ?= /usr/local
LIBDIR              ?= $(PREFIX)/lib
INCDIR              ?= $(PREFIX)/include
MANDIR              ?= $(PREFIX)/man/man3
DOCDIR              ?= $(PREFIX)/share/doc/libzeep-doc

BOOST_LIBS          = system thread filesystem regex random
BOOST_LIBS          := $(BOOST_LIBS:%=boost_%$(BOOST_LIB_SUFFIX))
LIBS                = $(BOOST_LIBS) stdc++ m pthread
LDFLAGS             += $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%) -g

VERSION_MAJOR       = 3.0
VERSION_MINOR       = 5
VERSION             = $(VERSION_MAJOR).$(VERSION_MINOR)
DIST_NAME           = libzeep-$(VERSION)
SO_NAME             = libzeep.so.$(VERSION_MAJOR)
LIB_NAME            = $(SO_NAME).$(VERSION_MINOR)

CXX                 ?= c++
CXXFLAGS            += -O2 $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -std=c++11 
CXXFLAGS            += -Wall
CXXFLAGS            += -g
LD                  ?= ld
LD_CONFIG			?= ldconfig

VPATH += src

OBJECTS = \
	obj/doctype.o \
	obj/document.o \
	obj/exception.o \
	obj/node.o \
	obj/soap-envelope.o \
	obj/parser.o \
	obj/reply.o \
	obj/request.o \
	obj/md5.o \
	obj/message_parser.o \
	obj/connection.o \
	obj/http-server.o \
	obj/preforked-http-server.o \
	obj/soap-server.o \
	obj/unicode_support.o \
	obj/webapp.o \
	obj/webapp-el.o \
	obj/xpath.o \
	obj/writer.o

lib: libzeep.a # libzeep.so

libzeep.a: $(OBJECTS)
	ar rc $@ $(OBJECTS)
	ranlib $@
#	ld -r -o $@ $(OBJECTS)

$(LIB_NAME): $(OBJECTS)
	$(CXX) -shared -o $@ -Wl,-soname=$(SO_NAME) $(OBJECTS) $(LDFLAGS)

$(SO_NAME): $(LIB_NAME)
	ln -fs $(LIB_NAME) $@

libzeep.so: $(SO_NAME)
	ln -fs $(LIB_NAME) $@

# assuming zeep-test is build when install was not done already
zeep-test: obj/zeep-test.o libzeep.a
	$(CXX) $(BOOST_INC_DIR:%=-I%) -o $@ -I. $^ $(LDFLAGS) -lboost_date_time -lboost_regex

install-libs: libzeep.so
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

install-doc: doc
	install -d $(MANDIR) $(DOCDIR)/html
	install doc/libzeep.3 $(MANDIR)/libzeep.3
	cd doc; for d in `find html -type d`; do install -d $(DOCDIR)/$$d; done
	cd doc; for f in `find html -type f`; do install $$f $(DOCDIR)/$$f; done

install: install-libs install-dev install-doc

dist: doc
	rm -rf $(DIST_NAME)
	mkdir -p $(DIST_NAME)
	git archive master | tar xC $(DIST_NAME)
	find doc/html -depth | cpio -pd $(DIST_NAME)
	tar czf $(DIST_NAME).tgz $(DIST_NAME)
	rm -rf $(DIST_NAME)

doc: FORCE
	cd doc; bjam

obj/%.o: %.cpp | obj
	$(CXX) -MD -c -o $@ $< $(CXXFLAGS)

obj:
	mkdir -p obj

-include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

test: libzeep.a
	make -C tests

clean:
	rm -rf obj/* libzeep.a libzeep.so* zeep-test $(DIST_NAME) $(DIST_NAME).tgz
	cd doc; bjam clean
	rm -rf doc/bin doc/html
	$(MAKE) -C tests clean

FORCE:
