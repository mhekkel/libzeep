# Makefile for libzeep and a test/sample application using libzeep.
#
#  Copyright Maarten L. Hekkelman, Radboud University 2008-2010.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# You may have to edit the first three defines on top of this
# makefile to match your current installation.

BOOST_LIB_SUFFIX	= -mt				# Works for Ubuntu
#BOOST_LIB_DIR		= /usr/local/lib/
#BOOST_INC_DIR		= /usr/local/include/boost-1_37

DESTDIR				?= /usr/local/
LIBDIR				= $(DESTDIR)lib
INCDIR				= $(DESTDIR)include
MANDIR				= $(DESTDIR)man/man3

BOOST_LIBS			= boost_system boost_thread
BOOST_LIBS			:= $(BOOST_LIBS:%=%$(BOOST_LIB_SUFFIX))
LIBS				= expat $(BOOST_LIBS)
LDOPTS				= $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%) -gdwarf-2

CC					?= c++
CFLAGS				= $(BOOST_INC_DIR:%=-I%) -iquote ./ -gdwarf-2 -fPIC

VPATH += src

OBJECTS = \
	obj/document.o \
	obj/exception.o \
	obj/node.o \
	obj/soap-envelope.o \
	obj/parser.o \
	obj/request_parser.o \
	obj/reply.o \
	obj/connection.o \
	obj/http-server.o \
	obj/soap-server.o

lib: libzeep.a libzeep.so

libzeep.a: $(OBJECTS)
	ld -r -o $@ $?

libzeep.so: $(OBJECTS)
	c++ -shared -o $@ $(LDOPTS) $?

zeep-test: lib obj/zeep-test.o
	c++ -o $@ $(OBJECTS) $(LDOPTS) libzeep.a

parser-test: lib obj/parser-test.o
	c++ -o $@ $(OBJECTS) $(LDOPTS)

install: lib
	install -d $(LIBDIR) $(MANDIR) $(INCDIR)/zeep/xml $(INCDIR)/zeep/http
	install ./libzeep.a $(LIBDIR)/libzeep.a
	install ./libzeep.so $(LIBDIR)/libzeep.so.1
	ln -fs $(LIBDIR)libzeep.so.1 $(LIBDIR)libzeep.so
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
	install zeep/envelope.hpp $(INCDIR)/zeep/envelope.hpp
	install zeep/exception.hpp $(INCDIR)/zeep/exception.hpp
	install zeep/dispatcher.hpp $(INCDIR)/zeep/dispatcher.hpp
	install zeep/server.hpp $(INCDIR)/zeep/server.hpp
	install doc/libzeep.3 $(MANDIR)/libzeep.3

obj/%.o: %.cpp
	c++ -MD -c -o $@ $< $(CFLAGS)

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

clean:
	rm -rf obj/* libzeep.a libzeep.so zeep-test
