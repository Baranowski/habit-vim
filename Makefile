CC=gcc

all: bin/wrapper bin/analyze

bin/wrapper:	src/wrapper.c
	gcc -O2 src/wrapper.c -lutil -o bin/wrapper

ANALYZE_OBJ = src/analyze.o \
	      src/config.o \
	      src/config_parse.o

ANALYZE_LIBS += $(shell pkg-config --libs yaml-cpp)

bin/analyze:	$(ANALYZE_OBJ)
	gcc -o $@ $(ANALYZE_OBJ) $(ANALYZE_LIBS)

DEPS = src/config.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

%.o: %.cpp $(DEPS)
	g++ -c -o $@ $<
