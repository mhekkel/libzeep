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
BOOST_LIB_DIR		= $(HOME)/projects/boost/lib
BOOST_INC_DIR		= $(HOME)/projects/boost/include

LIBDIR				= $(DESTDIR)lib
INCDIR				= $(DESTDIR)include
MANDIR				= $(DESTDIR)man/man3

BOOST_LIBS			= system filesystem thread regex program_options math_c99
BOOST_LIBS			:= $(BOOST_LIBS:%=boost_%$(BOOST_LIB_SUFFIX))
LIBS				= $(BOOST_LIBS)
LDOPTS				= ../libzeep.a $(BOOST_LIB_DIR:%=-L%) $(LIBS:%=-l%) -g -pthread

CC					?= c++
CFLAGS				= $(BOOST_INC_DIR:%=-I%) -iquote ../ -g -fPIC -pthread -std=c++0x

VPATH += src

test: parser-test xpath-test
	test -d XML-Test-Suite || tar xjf XML-Test-Suite.tbz
	./parser-test
	./xpath-test

parser-test: obj/parser-test.o
	$(CC) -o $@ $^ -L.. $(LDOPTS)

xpath-test: obj/xpath-test.o
	$(CC) -o $@ $? -L.. $(LDOPTS)

obj/%.o: %.cpp
	@ test -d obj || mkdir obj
	$(CC) -MD -c -o $@ -I .. $< $(CFLAGS)

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):

clean:
	rm -rf obj/* libzeep.a libzeep.so zeep-test
