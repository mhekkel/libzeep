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
DOCDIR              ?= $(PREFIX)/share/libzeep

BOOST_LIBS          = system thread filesystem regex math_c99
BOOST_LIBS          := $(BOOST_LIBS:%=boost_%$(BOOST_LIB_SUFFIX))
LIBS                = $(BOOST_LIBS) stdc++ m pthread rt
LDFLAGS             += $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%) -g

VERSION_MAJOR       = 3.0
VERSION_MINOR       = 2
VERSION             = $(VERSION_MAJOR).$(VERSION_MINOR)
DIST_NAME           = libzeep-$(VERSION)
SO_NAME             = libzeep.so.$(VERSION_MAJOR)
LIB_NAME            = $(SO_NAME).$(VERSION_MINOR)

CXX                 ?= c++
CFLAGS              += -O2 $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -std=c++11 -stdlib=libc++
#CFLAGS             += -g $(BOOST_INC_DIR:%=-I%) -I. -fPIC -pthread -shared # -std=c++0x
CFLAGS              += -Wall
CFLAGS              += -g
LD                  ?= ld

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
	ld -r -o $@ $(OBJECTS)

$(LIB_NAME): $(OBJECTS)
	$(CXX) -shared -o $@ -Wl,-soname=$(SO_NAME) $(LDFLAGS) $(OBJECTS)

$(SO_NAME): $(LIB_NAME)
	ln -fs $(LIB_NAME) $@

libzeep.so: $(SO_NAME)
	ln -fs $(LIB_NAME) $@

# assuming zeep-test is build when install was not done already
zeep-test: zeep-test.cpp libzeep.a
	$(CXX) $(BOOST_INC_DIR:%=-I%) -o $@ -I. zeep-test.cpp libzeep.a $(LDFLAGS) -lboost_date_time -lboost_regex

install-libs: libzeep.so
	install -d $(LIBDIR)
	install $(LIB_NAME) $(LIBDIR)/$(LIB_NAME)
	ln -Tfs $(LIB_NAME) $(LIBDIR)/$(SO_NAME)
	strip --strip-unneeded $(LIBDIR)/$(LIB_NAME)

install-dev:
	install -d $(MANDIR) $(LIBDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http $(INCDIR)/zeep/http/webapp
	install zeep/http/connection.hpp $(INCDIR)/zeep/http/connection.hpp
	install zeep/http/header.hpp $(INCDIR)/zeep/http/header.hpp
	install zeep/http/preforked-server.hpp $(INCDIR)/zeep/http/preforked-server.hpp
	install zeep/http/reply.hpp $(INCDIR)/zeep/http/reply.hpp
	install zeep/http/request.hpp $(INCDIR)/zeep/http/request.hpp
	install zeep/http/request_handler.hpp $(INCDIR)/zeep/http/request_handler.hpp
	install zeep/http/request_parser.hpp $(INCDIR)/zeep/http/request_parser.hpp
	install zeep/http/server.hpp $(INCDIR)/zeep/http/server.hpp
	install zeep/http/webapp.hpp $(INCDIR)/zeep/http/webapp.hpp
	install zeep/http/webapp/el.hpp $(INCDIR)/zeep/http/webapp/el.hpp
	install zeep/xml/doctype.hpp $(INCDIR)/zeep/xml/doctype.hpp
	install zeep/xml/document.hpp $(INCDIR)/zeep/xml/document.hpp
	install zeep/xml/node.hpp $(INCDIR)/zeep/xml/node.hpp
	install zeep/xml/parser.hpp $(INCDIR)/zeep/xml/parser.hpp
	install zeep/xml/serialize.hpp $(INCDIR)/zeep/xml/serialize.hpp
	install zeep/xml/unicode_support.hpp $(INCDIR)/zeep/xml/unicode_support.hpp
	install zeep/xml/writer.hpp $(INCDIR)/zeep/xml/writer.hpp
	install zeep/xml/xpath.hpp $(INCDIR)/zeep/xml/xpath.hpp
	install zeep/config.hpp $(INCDIR)/zeep/config.hpp
	install zeep/dispatcher.hpp $(INCDIR)/zeep/dispatcher.hpp
	install zeep/envelope.hpp $(INCDIR)/zeep/envelope.hpp
	install zeep/exception.hpp $(INCDIR)/zeep/exception.hpp
	install zeep/server.hpp $(INCDIR)/zeep/server.hpp
	install doc/libzeep.3 $(MANDIR)/libzeep.3
	for d in . images libzeep zeep zeep/http zeep/http/preforked_server_base zeep/http/el \
		zeep/http/el/object zeep/xml zeep/xml/doctype zeep/xml/container zeep/xml/element \
		index; do install -d $(DOCDIR)/$$d; install doc/html/$$d/*.* $(DOCDIR)/$$d; done;
	install ./libzeep.a $(LIBDIR)/libzeep.a
	strip -SX $(LIBDIR)/libzeep.a
	ln -Tfs $(LIB_NAME) $(LIBDIR)/libzeep.so

install: install-libs install-dev

dist: lib
	rm -rf $(DIST_NAME)
	svn export . $(DIST_NAME)
	find doc/html | grep -v '.svn' | cpio -pvd $(DIST_NAME)
	rm -rf $(DIST_NAME)/tests
	tar czf $(DIST_NAME).tgz $(DIST_NAME)
	rm -rf $(DIST_NAME)
	cp $(DIST_NAME).tgz ../ppa/libzeep_$(VERSION).orig.tar.gz

obj/%.o: %.cpp | obj
	$(CXX) -MD -c -o $@ $< $(CFLAGS)

obj:
	mkdir -p obj

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

test: libzeep.a
	make -C tests

clean:
	rm -rf obj/* libzeep.a libzeep.so* zeep-test $(DIST_NAME) $(DIST_NAME).tgz
