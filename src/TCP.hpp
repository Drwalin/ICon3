
#ifndef TCP_HPP
#define TCP_HPP

#include <string>
#include <vector>

#include "ASIO.hpp"
#include "Socket.hpp"

namespace tcp {
	
	class Socket : public asio::Socket<boost::asio::ip::tcp::socket> {
	public:
		Socket();
		virtual ~Socket() override;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		bool TryPopMessage(Message& message, int timeoutms=-1);
		void GetMessageCompletition(uint64_t&recvd, uint64_t& required);
		boost::asio::ip::tcp::socket* GetSocket();
		bool HasMessage() const;
		void Close();
		
		bool Connect(const Endpoint& endpoint);
		
		void CreateEmptySocket();
	};
	
	
	
	class Server : public asio::Server<Socket> {
	public:
		
		Server();
		virtual ~Server() override;
		
		void Open(const Endpoint& endpoint);
		virtual void Close() override;
		
		bool IsListening();
		bool HasNewSocket() const;
		Socket* TryGetNewSocket(int timeoutms=-1);
		Socket* GetNewSocket();
		void StartListening();
		
		virtual bool Valid() const override;
		
		virtual Socket* CreateEmptyAcceptingSocket() override;
		virtual bool Accept(Socket* socket) override;
	};
};

#endif

