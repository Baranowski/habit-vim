all: bin/wrapper

bin/wrapper:	src/wrapper.c
	gcc -O2 src/wrapper.c -lutil -o bin/wrapper
