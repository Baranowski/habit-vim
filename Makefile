CC=gcc

ANALYZE_OBJ = \
	      src/keys.o \
	      src/analyze.o \
	      src/config.o \
	      src/config_parse.o \

ANALYZE_LIBS += $(shell pkg-config --libs yaml-cpp)


all: bin/wrapper bin/analyze

bin/wrapper:	src/wrapper.c
	gcc -O2 src/wrapper.c -lutil -o bin/wrapper

bin/analyze:	$(ANALYZE_OBJ)
	gcc -o $@ $(ANALYZE_OBJ) $(ANALYZE_LIBS)


DEPS = src/config.h \
       src/keys.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

%.o: %.cpp $(DEPS)
	g++ -c -o $@ $<
