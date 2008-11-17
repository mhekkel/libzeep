VPATH += src

OBJECTS = \
	obj/document.o \
	obj/node.o \
	obj/exception.o \
	obj/soap-envelope.o \
	obj/soap-cgi.o

soap-cgi: $(OBJECTS)
	c++ -o $@ $(OBJECTS) -lexpat

obj/%.o: %.cpp
	c++ -MD -c -o $@ $< -I/usr/local/include/boost-1_36/ -iquote .

obj/%.o: src/%.cpp
	c++ -MD -c -o $@ $< -I/usr/local/include/boost-1_36/ -iquote .

include $(OBJECTS:%.o=%.d)

$(OBJECTS:.o=.d):
