
#ifndef TCP_HPP
#define TCP_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>

#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>

#include "ASIO.hpp"

#ifdef ASIO_CPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#else

#endif // ASIO_CPP


namespace TCP {
	class Socket {
	public:
		
		Socket();
		Socket(const Endpoint& endpoint);
		Socket(Socket&& other) :
			sock(other.sock), buffer(other.buffer) {
		}
		
		~Socket();
		
		bool Connect(const Endpoint& endpoint);
		void CreateEmpty();
		void Close();
		
		Endpoint GetEndpoint() const;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		void FetchData();
		
		bool HasMessage() const {
			return CanReadFullMessage(buffer);
		}
		
		float GetMessageCompletition(uint64_t&recvd, uint64_t& required) {
			FetchData();
			recvd = buffer.size();
			NumberBuffer n;
			uint64_t c = n.SetBytes(&buffer.front(), buffer.size());
			if(c == 0)
				return 0.0f;
			required = n.GetValue() + c;
			if(HasMessage()) {
				recvd = required;
				return 100.0f;
			}
			return 100.0 * (double)recvd / (double)required;
		}
		
		inline bool PopMessage(Message& message, int timeoutms=-1) {
			FetchData();
			if(timeoutms > 0) {
				int end = clock() + timeoutms;
				while(!HasMessage() && end>clock()) {
					std::this_thread::yield();
					FetchData();
					std::this_thread::yield();
				}
			}
			if(HasMessage()) {
				uint64_t readed = TryReadMessageFromBuffer(message, buffer);
				if(readed > 0) {
					buffer.erase(buffer.begin(), buffer.begin()+readed);
					if(buffer.capacity() > 1024*1024) {
						buffer.shrink_to_fit();
					}
				}
				return true;
			}
			return false;
		}
		
		bool Valid() const {
			return sock != NULL;
		}
		
		friend class Server;
		
	private:
		boost::asio::ip::tcp::socket* sock;
		std::vector<uint8_t> buffer;
	};
#ifdef ASIO_CPP
	Socket::Socket() {
		sock = NULL;
	}
	
	Socket::Socket(const Endpoint& endpoint) {
		sock = NULL;
		Connect(endpoint);
	}
	
	Socket::~Socket() {
		Close();
	}
	
	bool Socket::Connect(const Endpoint& endpoint) {
		CreateEmpty();
		boost::system::error_code err;
		sock->connect(endpoint.TcpEndpoint(), err);
		if(err) {
			delete sock;
			sock = NULL;
			return false;
		}
		return true;
	}
	
	void Socket::CreateEmpty() {
		Close();
		sock = new boost::asio::ip::tcp::socket(IoService());
	}
	
	void Socket::Close() {
		if(sock) {
			sock->close();
			delete sock;
			sock = NULL;
		}
		buffer.clear();
	}
	
	Endpoint Socket::GetEndpoint() const {
		if(sock)
			return Endpoint(sock->remote_endpoint());
		else
			return Endpoint();
	}
	
	bool Socket::Send(const std::vector<uint8_t>& buffer) {
		if(sock) {
			boost::system::error_code err;
			const uint64_t packetSize = 64*1024;
			for(uint64_t i=0; i<buffer.size();) {
				uint64_t toWrite = std::min<uint64_t>(
						buffer.size()-i,
						packetSize);
				uint64_t written = sock->write_some(
						boost::asio::buffer(
							&(buffer[i]),
							toWrite),
						err);
				
				i += written;
				if(written == 0)
					printf("\n fault send (0): %llu / %llu", i, buffer.size());
				if(err) {
					printf("\n fault send: %llu / %llu", i, buffer.size());
					return false;
				}
			}
			printf("\n send: %llu / %llu", buffer.size(), buffer.size());
			return true;
		}
		return false;
	}
	bool Socket::Send(const Message& msg) {
		if(sock) {
			std::vector<uint8_t> buffer;
			CreateOptimalBuffer(msg, buffer);
			return Send(buffer);
		}
		return false;
	}
	
	void Socket::FetchData() {
		if(sock) {
			uint64_t toRead = sock->available();
			if(toRead > 0) {
				boost::system::error_code err;
				uint64_t currentBufferSize = buffer.size();
				buffer.resize(currentBufferSize+toRead);
				uint64_t readed = sock->read_some(
						boost::asio::buffer(
							&(buffer[currentBufferSize]),
							toRead),
						err);
				if(readed != toRead) {
					buffer.resize(currentBufferSize+readed);
					DEBUG("Reading failed to read available data");
				}
				if(err)
					DEBUG("Receiving failed");
			}
		}
	}
#endif
	
	
	
	class Server {
	public:
		
		Server();
		~Server();
		
		void Open(const Endpoint& endpoint);
		void Close();
		
		bool Listen(Socket* socket);
		
		bool Valid() const {
			return acceptor != NULL;
		}
		
	private:
		boost::asio::ip::tcp::acceptor* acceptor;
	};
#ifdef ASIO_CPP
	Server::Server() {
		acceptor = NULL;
	}
	Server::~Server() {
		Close();
	}
	
	void Server::Open(const Endpoint& endpoint) {
		Close();
		acceptor =
			new boost::asio::ip::tcp::acceptor(
					IoService(),
					endpoint.TcpEndpoint());
	}
	
	void Server::Close() {
		if(acceptor) {
			acceptor->close();
			delete acceptor;
			acceptor = NULL;
		}
	}
	
	bool Server::Listen(Socket* socket) {
		if(Valid()) {
			socket->CreateEmpty();
			boost::system::error_code err;
			acceptor->accept(*(socket->sock), err);
			if(err) {
				socket->Close();
				return false;
			}
			return true;
		}
		return false;
	}
#endif
};

#endif // ASIO_HPP

