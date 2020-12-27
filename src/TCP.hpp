
#ifndef TCP_HPP
#define TCP_HPP

#include <string>
#include <vector>

#include "ASIO.hpp"

namespace tcp {
	
	class Socket {
	public:
		
		Socket();
		Socket(const Endpoint& endpoint);
		Socket(Socket&& other);
		
		~Socket();
		
		bool Connect(const Endpoint& endpoint);
		void CreateEmpty();
		void Close();
		
		Endpoint GetEndpoint() const;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		
		void FetchData();
		
		bool HasMessage() const;
		float GetMessageCompletition(uint64_t&recvd, uint64_t& required);
		bool PopMessage(Message& message, int timeoutms=-1);
		
		bool Valid() const;
		
		friend class Server;
		
	private:
		
		boost::asio::ip::tcp::socket* sock;
		std::vector<uint8_t> buffer;
	};
	
	
	
	class Server {
	public:
		
		Server();
		~Server();
		
		void Open(const Endpoint& endpoint);
		void Close();
		
		bool Listen(Socket* socket);
		
		bool Valid() const;
		
	private:
		
		boost::asio::ip::tcp::acceptor* acceptor;
	};
};

#endif

