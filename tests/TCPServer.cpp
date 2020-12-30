
#include <string>
#include <ctime>
#include <iostream>
#include <cstring>
#include <cstdio>

#include <TCP.hpp>

#include <windows.h>
#include <conio.h>

bool CheckChecksum(const std::vector<uint8_t>& buffer) {
	uint8_t checksum=0;
	for(uint64_t i=0; i+2<buffer.size(); ++i) {
		checksum ^= buffer[i];
	}
	return checksum == buffer[buffer.size()-1];
}

int main() {
	printf("\n Running!\n");
	
	tcp::Socket* socket = NULL;
	tcp::Server server;
	server.Open(Endpoint("", 27000));
	server.StartListening();
	
	if(socket = server.GetNewSocket()) {
		socket->Send(Message("Hello from server 1 !","fh4789w3784"));
		socket->Send(Message("Message from server 2 !","4ufr83u48f"));
		socket->Send(Message("Message from server 3 !","f4y789ffhF*(HW"));
		socket->Send(Message("Message from server 4 !"));
		
		Message msg;
		
		if(socket->TryPopMessage(msg, 10000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket->TryPopMessage(msg, 10000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket->TryPopMessage(msg, 10000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
	
		if(socket->TryPopMessage(msg, 10000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket->Send(Message("Message from server 5 !"));
		
		if(socket->TryPopMessage(msg, 10000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		while(true) {
			uint64_t recvd=0, req=0;
			IoContextPollOne();
			socket->GetMessageCompletition(recvd, req);
			if(recvd > 0)
				break;
		}
		
		int beg = clock();
		while(true) {
			IoContextPollOne();
			uint64_t recvd=0, req=0;
			socket->GetMessageCompletition(recvd, req);
			printf("\n received: %2.2f%% = ",
					(float)(recvd+1)*100.0f/(float)(req+1));
			printf(" %llu / %llu bytes", recvd, req);
			printf("   speed = %.2f MiB/s", (float)(recvd)*1000.0f/1024.0f/1024.0f/(float)(clock()-beg));
			if(socket->TryPopMessage(msg, 3000)) {
				recvd = msg.title.size() + 1 + msg.data.size() + 2;
				printf("\n received: %s", msg.title.c_str());
				printf("   speed = %.2f MiB/s", (float)(recvd)*1000.0f/1024.0f/1024.0f/(float)(t));
				printf("\n checksum: %s", CheckChecksum(msg.data)?"true":"false");
				break;
			}
		}
		
		socket->Send(Message("Message from server 6 !"));
		
		socket->Close();
		delete socket;
		socket = NULL;
	} else {
		printf("\n cannot connect");
	}
	server.Close();
	
	getch();
	
	return 0;
}

