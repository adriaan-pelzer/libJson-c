CFLAGS=-g -Wall -fPIC -std=c99 -I/usr/local/include/json-c
LIBS=-L/usr/local/lib -ljson-c

all: libJson

test: testtweets
	valgrind --leak-check=full ./testtweets

testtweets: Makefile testtweets.o json.o json.h tweets.h addr.h
	gcc -o testtweets testtweets.o -L/usr/local/lib -lJson

libJson: Makefile json.o json.h addr.h
	gcc -shared -W1,-soname,libJson.so.1 -o libJson.so.1.0 json.o ${LIBS}

testtweets.o: Makefile testtweets.c tweets.h addr.h json.h
	gcc ${CFLAGS} -c testtweets.c -o testtweets.o

json.o: Makefile json.h json.c addr.h
	gcc ${CFLAGS} -c json.c -o json.o

install:
	cp libJson.so.1.0 /usr/local/lib
	ln -sf /usr/local/lib/libJson.so.1.0 /usr/local/lib/libJson.so.1
	ln -sf /usr/local/lib/libJson.so.1.0 /usr/local/lib/libJson.so
	cp json.h /usr/local/include/Json.h
	ldconfig /usr/local/lib

clean:
	rm *.o; rm *.so*; rm testtweets;
