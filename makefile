LDOPTS = -lexpat -L/usr/local/lib -lboost_system-gcc43-mt-d-1_36  -lboost_thread-gcc43-mt-d-1_36

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
	obj/soap-cgi.o

soap-cgi: $(OBJECTS)
	c++ -o $@ $(OBJECTS) $(LDOPTS)

obj/%.o: %.cpp
	c++ -MD -c -o $@ $< -I/usr/local/include/boost-1_36/ -iquote .

obj/%.o: src/%.cpp
	c++ -MD -c -o $@ $< -I/usr/local/include/boost-1_36/ -iquote .

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):
