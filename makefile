# Makefile for libzeep and a test/sample application using libzeep.
#
#  Copyright Maarten L. Hekkelman, Radboud University 2008.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# You may have to edit the first three defines on top of this
# makefile to match your current installation.

BOOST_LIB_SUFFIX	= -mt				# Works for Ubuntu
#BOOST_LIB_DIR		= /usr/local/lib/
#BOOST_INC_DIR		= /usr/local/include/boost-1_37

DESTDIR				= /usr/local

BOOST_LIBS			= boost_system boost_thread
BOOST_LIBS			:= $(BOOST_LIBS:%=%$(BOOST_LIB_SUFFIX))
LIBS				= expat $(BOOST_LIBS)
LDOPTS				= $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%)

CFLAGS				= $(BOOST_INC_DIR:%=-I%) -iquote ./ -gdwarf-2 -fPIC

VPATH += src

OBJECTS = \
	obj/document.o \
	obj/exception.o \
	obj/node.o \
	obj/soap-envelope.o \
	obj/request_parser.o \
	obj/reply.o \
	obj/connection.o \
	obj/http-server.o \
	obj/soap-server.o

lib: libzeep.a

libzeep.a: $(OBJECTS)
	ld -r -o libzeep.a $(OBJECTS)

zeep-test: lib obj/zeep-test.o
	c++ -o $@ $(OBJECTS) $(LDOPTS) libzeep.a

install: lib
	install libzeep.a $(DESTDIR)/lib/libzeep.a
	install -d $(DESTDIR)/include/zeep/xml $(DESTDIR)/include/zeep/http
	install zeep/http/reply.hpp $(DESTDIR)/include/zeep/http/reply.hpp
	install zeep/http/connection.hpp $(DESTDIR)/include/zeep/http/connection.hpp
	install zeep/http/request_parser.hpp $(DESTDIR)/include/zeep/http/request_parser.hpp
	install zeep/http/request_handler.hpp $(DESTDIR)/include/zeep/http/request_handler.hpp
	install zeep/http/server.hpp $(DESTDIR)/include/zeep/http/server.hpp
	install zeep/http/header.hpp $(DESTDIR)/include/zeep/http/header.hpp
	install zeep/http/request.hpp $(DESTDIR)/include/zeep/http/request.hpp
	install zeep/xml/document.hpp $(DESTDIR)/include/zeep/xml/document.hpp
	install zeep/xml/node.hpp $(DESTDIR)/include/zeep/xml/node.hpp
	install zeep/xml/serialize.hpp $(DESTDIR)/include/zeep/xml/serialize.hpp
	install zeep/envelope.hpp $(DESTDIR)/include/zeep/envelope.hpp
	install zeep/exception.hpp $(DESTDIR)/include/zeep/exception.hpp
	install zeep/dispatcher.hpp $(DESTDIR)/include/zeep/dispatcher.hpp
	install zeep/server.hpp $(DESTDIR)/include/zeep/server.hpp

obj/%.o: %.cpp
	c++ -MD -c -o $@ $< $(CFLAGS)

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

clean:
	rm -rf obj/*
