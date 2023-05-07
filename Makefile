CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

.PHONY: build clean

build: server subscriber

server:
	gcc $(CFLAGS) -o server server.cpp common.cpp -lstdc++ -lm

subscriber:
	gcc $(CFLAGS) -o subscriber subscriber.cpp common.cpp -lstdc++ -lm

clean:
	rm -rf server subscriber
