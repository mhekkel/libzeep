# Makefile for a test/sample application using libzeep.
#
#  Copyright Maarten L. Hekkelman, Radboud University 2008.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# You may have to edit the first three defines on top of this
# makefile to match your current installation.

BOOST_LIB_SUFFIX	= -gcc40-mt-d-1_37
BOOST_LIB_DIR		= /usr/local/lib/
BOOST_INC_DIR		= /usr/local/include/boost-1_37

BOOST_LIBS			= boost_system boost_thread
BOOST_LIBS			:= $(BOOST_LIBS:%=%$(BOOST_LIB_SUFFIX))
LIBS				= expat $(BOOST_LIBS)
LDOPTS				= $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%)

CFLAGS				= $(BOOST_INC_DIR:%=-I%) -iquote ./

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
	obj/soap-server.o \
	obj/zeep-test.o

zeep: $(OBJECTS)
	c++ -o $@ $(OBJECTS) $(LDOPTS)

obj/%.o: %.cpp
	c++ -MD -c -o $@ $< $(CFLAGS)

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

clean:
	rm -rf obj/*
