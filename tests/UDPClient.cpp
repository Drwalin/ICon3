
#include <array>
#include <string>
#include <iostream>
#include <cstdio>

#include <windows.h>

#include <UDP.hpp>

int main() {
	
	udp::Socket socket;
	socket.Open(Endpoint());
	Endpoint endpoint("127.0.0.1:27000");
	Message msg;
	uint64_t id=0;
	
	socket.Send(Message("Hello from client 1 !"), endpoint);
	socket.Send(Message("Hello from client 2 !"), endpoint);
	
	if(socket.PopAnyMessage(msg, id, 10000))
		printf("\n received: %s", msg.title.c_str());
	else
		printf("\n timedout");
	
	if(socket.PopAnyMessage(msg, id, 10000))
		printf("\n received: %s", msg.title.c_str());
	else
		printf("\n timedout");
	
	return 0;
}

