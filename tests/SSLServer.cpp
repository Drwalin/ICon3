
#include <string>
#include <ctime>
#include <iostream>
#include <cstring>
#include <cstdio>

#include <windows.h>
#include <conio.h>

#include <SSL.hpp>

bool CheckChecksum(const std::vector<uint8_t>& buffer) {
	uint8_t checksum=0;
	for(uint64_t i=0; i+2<buffer.size(); ++i) {
		checksum ^= buffer[i];
	}
	return checksum == buffer[buffer.size()-1];
}

int main() {
	printf("\n Running!\n");
	
	ssl::Socket socket;
	ssl::Server server;
	server.Open(Endpoint("", 27000),
			"cert/user.crt",
			"cert/user.key",
			"cert/dh2048.pem");
	if(server.Listen(&socket)) {
		socket.Send(Message("Hello from server 1 !","fh4789w3784"));
		socket.Send(Message("Message from server 2 !","4ufr83u48f"));
		socket.Send(Message("Message from server 3 !","f4y789ffhF*(HW"));
		socket.Send(Message("Message from server 4 !"));
		
		Message msg;
		
		if(socket.PopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket.PopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket.PopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket.PopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket.Send(Message("Message from server 5 !"));
		
		if(socket.PopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket.Send(Message("Message from server 6 !"));
		
		while(true) {
			socket.Poll();
			uint64_t recvd=0, req=0;
			socket.GetMessageCompletition(recvd, req);
			if(recvd > 0)
				break;
		}
		
		int beg = clock();
		while(true) {
			uint64_t recvd=0, req=0;
			printf("\n received: %2.2f%% = ",
					socket.GetMessageCompletition(recvd, req));
			printf(" %llu / %llu bytes", recvd, req);
			printf("   speed = %.2f MiB/s", (float)(recvd)*1000.0f/1024.0f/1024.0f/(float)(clock()-beg));
			if(socket.PopMessage(msg, 3000)) {
				printf("\n   speed = %.2f MiB/s", (float)(msg.title.size()+msg.data.size())*1000.0f/1024.0f/1024.0f/(float)(clock()-beg));
				printf("\n received: %s", msg.title.c_str());
				printf("\n checksum: %s", CheckChecksum(msg.data)?"true":"false");
				break;
			}
		}
		socket.Close();
	} else {
		printf("\n cannot connect");
	}
	printf("\n Done!");
	server.Close();
	
	getch();
	
	return 0;
}

