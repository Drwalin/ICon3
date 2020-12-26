
CFLAGS = -Ofast -s -m64 -pipe
CMPFLAGS = -IC:\Programs\mingw-w64\include -Isrc
LDFLAGS = -LC:\Programs\mingw-w64\lib
LDFLAGS += -lwinmm -lWs2_32 -lMswsock -lAdvApi32 -lmsvcrt -lpthread -lcrypto -lssl
CC = g++

objects = bin/ASIO.obj bin/TCP.obj bin/UDP.obj

all: $(objects) $(exes) udp tcp
udp: UDPServer.exe UDPClient.exe
tcp: TCPServer.exe TCPClient.exe
pureudp: PureUDPServer.exe PureUDPClient.exe
puressl: PureSSLServer.exe PureSSLClient.exe
puressl2: PureSSLServer2.exe PureSSLClient2.exe

%.exe: bin/%.obj $(objects)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(CMPFLAGS)

Pure%.exe: tests/Pure%.cpp
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(CMPFLAGS)

%.exe: tests/%.cpp $(objects)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(CMPFLAGS)

bin/%.obj: src/%.cpp
	$(CC) -c $^ -o $@ $(CFLAGS) $(CMPFLAGS)

.PHONY: clean
clean:
	rm *.exe -f
	rm bin/*.obj -f

