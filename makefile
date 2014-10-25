client.o: server.o
	g++ -Wall -o  client  client.cpp
server.o: 
	g++ -Wall -Wno-sign-compare  -o  proxy  proxy.cpp
clean:
	rm -rf server
	rm -rf client
