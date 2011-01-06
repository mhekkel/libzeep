# Makefile for libzeep and a test/sample application using libzeep.
#
#  Copyright Maarten L. Hekkelman, Radboud University 2008-2010.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# You may have to edit the first three defines on top of this
# makefile to match your current installation.

#BOOST_LIB_SUFFIX	= -mt				# Works for Ubuntu
#BOOST_LIB_DIR		= $(HOME)/projects/boost/lib
#BOOST_INC_DIR		= $(HOME)/projects/boost/include

# the debian package building tools broke my build rules...
# DESTDIR				?= /usr/local/
LIBDIR				= $(DESTDIR)/usr/lib
INCDIR				= $(DESTDIR)/usr/include
MANDIR				= $(DESTDIR)/usr/man/man3

BOOST_LIBS			= boost_system boost_thread
BOOST_LIBS			:= $(BOOST_LIBS:%=%$(BOOST_LIB_SUFFIX))
LIBS				= $(BOOST_LIBS)
LDOPTS				= $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%) -g

LIB_SO_MAJOR		= 2
LIB_SO_MINOR		= 0
LIB_SO_NAME			= libzeep.so.$(LIB_SO_MAJOR)
LIB_NAME			= $(LIB_SO_NAME).$(LIB_SO_MINOR)

CC					?= c++
CFLAGS				= $(BOOST_INC_DIR:%=-I%) -iquote ./ -gdwarf-2 -fPIC -O3 -pthread -shared

VPATH += src

OBJECTS = \
	obj/doctype.o \
	obj/document.o \
	obj/exception.o \
	obj/node.o \
	obj/soap-envelope.o \
	obj/parser.o \
	obj/request_parser.o \
	obj/reply.o \
	obj/connection.o \
	obj/http-server.o \
	obj/preforked-http-server.o \
	obj/soap-server.o \
	obj/unicode_support.o \
	obj/xpath.o \
	obj/writer.o

lib: libzeep.a libzeep.so

libzeep.a: $(OBJECTS)
	ld -r -o $@ $(OBJECTS)

$(LIB_SO_NAME): $(LIB_NAME);	
	ln -fs $< $@

$(LIB_NAME): $(OBJECTS)
	$(CC) -shared -o $@ -Wl,-soname=$(LIB_SO_NAME) $(LDOPTS) $?

libzeep.so:  $(LIB_SO_NAME)
	ln -fs $< $@

zeep-test: lib obj/zeep-test.o
	c++ -o $@ $(LDOPTS) -lboost_filesystem -lzeep -L. -Wl,-rpath=. obj/zeep-test.o

install-libs: libzeep.a libzeep.so
	install -d $(LIBDIR)
	install ./libzeep.a $(LIBDIR)/libzeep.a
	install ./libzeep.so $(LIBDIR)/libzeep.so.1
	cd $(LIBDIR); ln -fs libzeep.so.1 libzeep.so

install-dev:
	install -d $(MANDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http
	install zeep/http/reply.hpp $(INCDIR)/zeep/http/reply.hpp
	install zeep/http/connection.hpp $(INCDIR)/zeep/http/connection.hpp
	install zeep/http/request_parser.hpp $(INCDIR)/zeep/http/request_parser.hpp
	install zeep/http/request_handler.hpp $(INCDIR)/zeep/http/request_handler.hpp
	install zeep/http/server.hpp $(INCDIR)/zeep/http/server.hpp
	install zeep/http/header.hpp $(INCDIR)/zeep/http/header.hpp
	install zeep/http/request.hpp $(INCDIR)/zeep/http/request.hpp
	install zeep/xml/document.hpp $(INCDIR)/zeep/xml/document.hpp
	install zeep/xml/node.hpp $(INCDIR)/zeep/xml/node.hpp
	install zeep/xml/serialize.hpp $(INCDIR)/zeep/xml/serialize.hpp
	install zeep/xml/parser.hpp $(INCDIR)/zeep/xml/parser.hpp
	install zeep/xml/unicode_support.hpp $(INCDIR)/zeep/xml/unicode_support.hpp
	install zeep/xml/doctype.hpp $(INCDIR)/zeep/xml/doctype.hpp
	install zeep/xml/xpath.hpp $(INCDIR)/zeep/xml/xpath.hpp
	install zeep/xml/writer.hpp $(INCDIR)/zeep/xml/writer.hpp
	install zeep/envelope.hpp $(INCDIR)/zeep/envelope.hpp
	install zeep/exception.hpp $(INCDIR)/zeep/exception.hpp
	install zeep/dispatcher.hpp $(INCDIR)/zeep/dispatcher.hpp
	install zeep/server.hpp $(INCDIR)/zeep/server.hpp
	install doc/libzeep.3 $(MANDIR)/libzeep.3

install: install-libs install-dev

obj/%.o: %.cpp
	c++ -MD -c -o $@ $< $(CFLAGS)

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

clean:
	rm -rf obj/* libzeep.a libzeep.so zeep-test
