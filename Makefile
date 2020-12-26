
CFLAGS = -Ofast -s -m64 -pipe
CMPFLAGS = -IC:\Programs\mingw-w64\include -Isrc
LDFLAGS = -lwinmm -lWs2_32 -lMswsock -lAdvApi32 -lmsvcrt -lpthread
CC = g++

objects = bin/ASIO.obj

all: $(objects) $(exes) udp tcp
udp: UDPServer.exe UDPClient.exe
tcp: TCPServer.exe TCPClient.exe
pure: PureUDPServer.exe PureUDPClient.exe

%.exe: bin/%.obj $(objects)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(CMPFLAGS)

%.exe: tests/%.cpp $(objects)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS) $(CMPFLAGS)

bin/%.obj: src/%.cpp
	$(CC) -c $^ -o $@ $(CFLAGS) $(CMPFLAGS)

.PHONY: clean
clean:
	del *.exe
	del bin\*.obj

