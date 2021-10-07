
#include <string>
#include <ctime>
#include <iostream>
#include <cstring>
#include <cstdio>

#include "../src/SSL.hpp"

template<typename T>
bool CheckChecksum(const T& buffer) {
	uint8_t checksum=0;
	for(uint64_t i=0; i+2<buffer.size(); ++i) {
		checksum ^= buffer[i];
	}
	return checksum == buffer[buffer.size()-1];
}

int main() {
	printf("\n Running!\n");
	
	ssl::Socket* socket = NULL;
	ssl::Server server;
	server.Open(Endpoint("", 27000),
			"cert/user.crt",
			"cert/user.key",
			"cert/dh2048.pem");
	server.StartListening();
	if(socket = server.GetNewSocket()) {
		socket->Send(Message("Hello from server 1 !","fh4789w3784"));
		socket->Send(Message("Message from server 2 !","4ufr83u48f"));
		socket->Send(Message("Message from server 3 !","f4y789ffhF*(HW"));
		socket->Send(Message("Message from server 4 !"));
		
		Message msg;
		
		if(socket->TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket->TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket->TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket->TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket->Send(Message("Message from server 5 !"));
		
		if(socket->TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket->Send(Message("Message from server 6 !"));
		
		while(true) {
			IoContextPollOne();
			uint64_t recvd=0, req=0;
			socket->GetMessageCompletition(recvd, req);
			if(recvd > 0)
				break;
		}
		
		int beg = clock();
		long long cooldown = clock();
		while(true) {
			uint64_t recvd=0, req=0;
			socket->GetMessageCompletition(recvd, req);
			if((clock()-cooldown)*4/CLOCKS_PER_SEC > 1) {
				cooldown = clock();
			printf("\n received: %2.2f%% = ",
					(float)(recvd+1)*100.0f/(float)(req+1));
			printf(" %llu / %llu bytes", recvd, req);
			printf("   speed = %.2f MiB/s", (float)(recvd)*(CLOCKS_PER_SEC)/1024.0f/1024.0f/(float)(clock()-beg));
			}
			if(socket->TryPopMessage(msg, 3000)) {
				printf("\n   speed = %.2f MiB/s", (float)(msg.title.size()+msg.data.size())*(CLOCKS_PER_SEC)/1024.0f/1024.0f/(float)(clock()-beg));
				printf("\n received: %s", msg.title.c_str());
				printf("\n checksum: %s", CheckChecksum(msg.data)?"true":"false");
				break;
			}
		}
		socket->Close();
		delete socket;
		socket = NULL;
	} else {
		printf("\n cannot connect");
	}
	printf("\n Done!");
	server.Close();
	
	int i;
	scanf("%i", &i);
	
	return 0;
}

