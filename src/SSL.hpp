
#ifndef SSL_HPP
#define SSL_HPP

#include <string>
#include <vector>
#include <queue>

#include "ASIO.hpp"

namespace ssl {
	
	class Socket {
	public:
		
		Socket();
		Socket(const Endpoint& endpoint, const char* certificateFile);
		Socket(Socket&&) = delete;
		Socket(const Socket&) = delete;
		Socket& operator =(const Socket&) = delete;
		
		~Socket();
		
		bool Connect(const Endpoint& endpoint, const char* certificateFile);
		void CreateEmpty(boost::asio::ssl::context* sslContext,
				boost::asio::io_context* ioContext);
		bool StartServerSide();
		void Close();
		
		Endpoint GetEndpoint() const;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		
		bool HasMessage() const;
		float GetMessageCompletition(uint64_t&recvd, uint64_t& required);
		bool PopMessage(Message& message, int timeoutms=-1);
		
		bool Valid() const;
		
		void Poll();
		
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* GetSocket();
		
		friend class Server;
		
	private:
		
#ifdef CPP_FILES_CPP
		void FetchData(const boost::system::error_code& err, size_t length);
		void CallRequest(uint64_t bytes);
#endif
		
		bool ioContextReference;
		boost::asio::io_context* ioContext;
		boost::asio::ssl::context* sslContext;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* sock;
		std::vector<uint8_t> buffer;
		std::queue<Message> receivedMessages;
		uint64_t fetchRequestSize;
	};
	
	
	
	class Server {
	public:
		
		Server();
		~Server();
		
		void Open(const Endpoint& endpoint, const char* certChain,
				const char* privateKey, const char* dhFile);
		void Close();
		
		// I have no idea what is the purpose of this method, but all examples
		// had it returning "test" and set as context->set_password_callback()
		std::string GetPassword() const;
		
		bool Listen(Socket* socket);
		
		bool Valid() const;
		
	private:
		
		boost::asio::ip::tcp::acceptor* acceptor;
		boost::asio::ssl::context* sslContext;
		boost::asio::io_context* ioContext;
	};
};

#endif

