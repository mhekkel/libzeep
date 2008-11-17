VPATH += src

OBJECTS = \
	obj/document.o \
	obj/node.o \
	obj/exception.o \
	obj/soap-envelope.o \
	obj/soap-cgi.o

soap-cgi: $(OBJECTS)
	c++ -o $@ $? -lexpat

obj/%.o: %.cpp
	c++ -c -o $@ $< -I/usr/local/include/boost-1_36/ -iquote .

obj/%.o: src/%.cpp
	c++ -c -o $@ $< -I/usr/local/include/boost-1_36/ -iquote .

