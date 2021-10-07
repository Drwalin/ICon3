
#include <string>
#include <cstdio>
#include <ctime>
#include <cstdlib>

#include "../src/SSL.hpp"

uint16_t Random(uint64_t& gen, uint64_t i) {
	gen = (gen<<4) ^ (gen>>7) ^ i;
	gen = (125213421^gen) + i + gen + (i^gen);
	gen = gen + 124241;
	gen += gen ^ 99429;
	return (uint16_t)(gen ^ (gen>>16) ^ (gen>>32) ^ (gen>>48));
}

std::vector<uint8_t> RandomData(uint64_t c) {
	if(c&7)
		c += 8- (c&7);
	uint64_t rndstate = (rand()+rand())*(rand()+rand()) + rand() + rand();
	std::vector<uint8_t> r;
	r.reserve(c+1);
	r.resize(c);
	uint64_t C = c;
	uint8_t checksum=0;
	int beg = clock();
	int BEG = clock();
	for(uint64_t i=0; i<r.size(); i+=8) {
		c = r[i] = i;
		checksum ^= c;
	}
	r.push_back(checksum);
	return r;
}

int main() {
	
	printf("\n Running!\n");
	
	srand(time(NULL));
	
	std::vector<uint8_t> uberMessageBuffer;
	{
		Message uberMessage("Message from client 6 !", RandomData(1024ll*1024ll*3ll));
		CreateOptimalBuffer(uberMessage, uberMessageBuffer);
	}
	
	printf("\n uber message created in %.2fs!", (float)(clock())/CLOCKS_PER_SEC);
	
	ssl::Socket socket;
	Endpoint endpoint("127.0.0.1:27000");
	if(socket.Connect(endpoint, "cert/rootca.crt")) {
		socket.Send(Message("Hello from client 1 !"));
		socket.Send(Message("Message from client 2 !"));
		socket.Send(Message("Message from client 3 !"));
		
		Message msg;
	
		if(socket.TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket.TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		if(socket.TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket.Send(Message("Message from client 4 !"));
		
		if(socket.TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
		
		socket.Send(Message("Message from client 5 !"));
		
		if(socket.TryPopMessage(msg, 1000))
			printf("\n received: %s", msg.title.c_str());
		else
			printf("\n timedout");
			
		bool s = socket.Send(uberMessageBuffer);
		printf("\n last big send: %s", s?"true":"false");
		{
			Message msg;
			int beg = clock();
			while(true) {
				if(socket.TryPopMessage(msg, 3000)) {
					printf("\n received: %s", msg.title.c_str());
					break;
				} else {
					uint64_t recvd=0, req=0;
					socket.GetMessageCompletition(recvd, req);
					printf("\n timedout: %.2fs: %f%%",
							(float)(clock()-beg)*0.001f,
							(float)(recvd+1)*100.0f/(float)(req+1));
					printf("\n received: %llu / %llu bytes", recvd, req);
				}
			}
		}
		socket.Close();
	} else {
		printf("\n cannot connect");
	}
	
	printf("\n Done!");
	int i;
	scanf("%i", &i);
	
	return 0;
}

