CFLAGS=-g
all: player.cpp dumb.cpp screw.cpp server
	g++ $(CFLAGS) player.cpp -o player
	g++ $(CFLAGS) dumb.cpp -o dumb
	g++ $(CFLAGS) screw.cpp -o screw

server: server.cpp
	g++ server.cpp $(CFLAGS) `xmlrpc-c-config c++2 abyss-server --cflags --libs` -o server

clean:
	rm -f server player dumb screw
