
#include <string>
#include <ctime>
#include <iostream>
#include <cstring>
#include <cstdio>

#include <windows.h>

#include <UDP.hpp>

extern "C" const char * str;

int main() {
	printf("\n Running!\n");
	
	udp::Socket socket, ss;
	socket.Open(Endpoint("", 27000));
	ss.Open(Endpoint());
	
	uint64_t id=0;
	Message msg;
	
	if(socket.PopAnyMessage(msg, id, 30000))
		printf("\n received: %s", msg.title.c_str());
	else
		printf("\n timedout");
	
	socket.Send(Message("Hello from server 1 !"), id);
	socket.Send(Message(str), socket.GetEndpoint(id));
	
	if(socket.PopAnyMessage(msg, id, 10000))
		printf("\n received: %s", msg.title.c_str());
	else
		printf("\n timedout");
	
	return 0;
}

const char * str = "Jakis troche wiekszy tekst wsadzony";

