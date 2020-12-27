
#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "ASIO.hpp"

#include <vector>
#include <queue>

namespace asio {
	
	template<typename T>
	class Socket {
	public:
		
		const static uint64_t maxSinglePacketSize = 64*1024;
		
		Socket();
		~Socket();
		
		virtual void Close()=0;
		
		const Endpoint& GetEndpoint() const;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		
		bool HasMessage() const;
		void GetMessageCompletition(uint64_t&recvd, uint64_t& required);
		void GetBufferMessageCompletition(uint64_t&recvd, uint64_t& required);
		bool TryPopMessage(Message& message, int timeoutms=-1);
		
		T* GetSocket();
		
		bool Valid() const;
		
	protected:
		
		void FetchData(const boost::system::error_code& err, size_t length);
		void RequestDataFetch(uint64_t bytes);
		
		T* socket;
		Endpoint endpoint;
		std::vector<uint8_t> buffer;
		std::queue<Message> receivedMessages;
		uint64_t fetchRequestSize;
	};
	
	template<typename T>
	class Server {
	public:
		
		Server();
		~Server();
		
		void Open(const Endpoint& endpoint);
		virtual void Close();
		
		bool IsListening();
		bool HasNewSocket() const;
		T* TryGetSocket(int timeoutms=-1);
		void StartListening();
		
		virtual bool Valid();
		
		
	protected:
		
		virtual T* CreateEmptyAcceptingSocket()=0;
		
#ifdef SOCKET_CPP
		void DoAccept(const boost::system::error_code& err);
#endif
		virtual bool Accept(T* socket);
		
		boost::asio::ip::tcp::acceptor* acceptor;
		std::queue<T*> newSockets;
		T* currentAcceptingSocket;
	};
};

#endif

