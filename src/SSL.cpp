
#ifndef CPP_FILES_CPP
#define CPP_FILES_CPP
#endif

#include <boost/asio/ssl.hpp>

#include "SSL.hpp"

#include <functional>
#include <thread>
#include <iostream>

namespace ssl {
	
	Socket::Socket() {
		sock = NULL;
		sslContext = NULL;
		fetchRequestSize = 0;
		ioContext = NULL;
		ioContextReference = false;
	}
	
	Socket::Socket(const Endpoint& endpoint, const char* certificateFile) {
		sock = NULL;
		sslContext = NULL;
		fetchRequestSize = 0;
		ioContext = NULL;
		ioContextReference = false;
		Connect(endpoint, certificateFile);
	}
	
	Socket::~Socket() {
		Close();
	}
	
	bool Socket::Connect(const Endpoint& endpoint,
			const char* certificateFile) {
		Close();
		ioContext = new boost::asio::io_context();
		ioContextReference = false;
		printf("\n Connecting");
		sslContext = new boost::asio::ssl::context(
				boost::asio::ssl::context::sslv23);
		sslContext->load_verify_file(certificateFile);
		sock = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(
				*ioContext, *sslContext);
		sock->set_verify_mode(boost::asio::ssl::verify_peer);
		sock->set_verify_callback(
				[](bool preverified, boost::asio::ssl::verify_context& ctx) {
					char subject_name[256];
					X509* cert = X509_STORE_CTX_get_current_cert(
							ctx.native_handle());
					X509_NAME_oneline(X509_get_subject_name(cert),
							subject_name, 256);
					printf("Verifying: %s ", subject_name);
					printf("\n preverified: %s", preverified?"true":"false");
					return preverified;
				});
		boost::system::error_code err;
		sock->lowest_layer().connect(endpoint.TcpEndpoint(), err);
		if(err) {
			fprintf(stderr, "\n Failed to connect: %s", err.message());
			fflush(stderr);
			delete sock;
			sock = NULL;
			return false;
		}
		sock->handshake(boost::asio::ssl::stream_base::client, err);
		if(err) {
			fprintf(stderr, "\n Failed to handshake: %s", err.message());
			fflush(stderr);
			delete sock;
			sock = NULL;
			return false;
		}
		CallRequest(1);
		printf("\n Connected");
		ioContext->poll_one();
		printf(" ...!");
		return true;
	}
	
	void Socket::CreateEmpty(boost::asio::ssl::context* sslContext,
			boost::asio::io_context* ioContext) {
		printf("\n Accepting");
		Close();
		ioContextReference = true;
		this->ioContext = ioContext;
		this->sock = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(
				*ioContext, *sslContext);
	}
	
	bool Socket::StartServerSide() {
		if(sock) {
			boost::system::error_code err;
			sock->handshake(boost::asio::ssl::stream_base::server, err);
			if(err) {
				Close();
				printf("\n Failed to handshake");
				return false;
			}
			CallRequest(1);
			printf("\n Accepted");
			return true;
		}
		printf("\n Failed to accept");
		return false;
	}
	
	void Socket::Close() {
		fetchRequestSize = 0;
		if(sock)
			delete sock;
		sock = NULL;
		if(sslContext)
			delete sslContext;
		if(ioContext && ioContextReference==false)
			delete ioContext;
		ioContext = NULL;
		ioContextReference = false;
		sslContext = NULL;
		buffer.clear();
		while(!receivedMessages.empty())
			receivedMessages.pop();
	}
	
	Endpoint Socket::GetEndpoint() const {
		if(sock)
			return Endpoint(sock->lowest_layer().remote_endpoint());
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
	
	void Socket::FetchData(const boost::system::error_code& err,
			size_t length) {
		if(sock) {
			if(fetchRequestSize != length) {
				buffer.resize(buffer.size()-(fetchRequestSize-length));
			}
			fetchRequestSize = 0;
			if(!err) {
				uint64_t recvd=0, required=0;
				GetMessageCompletition(recvd, required);
				if(required>0 && recvd>=required) {
					Message message;
					uint64_t readed = TryReadMessageFromBuffer(message, buffer);
					if(readed > 0) {
						receivedMessages.emplace();
						receivedMessages.back().Swap(message);
						buffer.erase(buffer.begin(), buffer.begin()+readed);
						if(buffer.capacity() > 64*1024)
							buffer.shrink_to_fit();
					} else {
						DEBUG("Failed to read message from buffer, when whole available");
					}
				} else if(required>0 && recvd<required) {
					CallRequest(required-recvd);
					return;
				}
			}
			CallRequest(1);
		} else {
			printf("\n socket is invalid!");
		}
	}
	
	void Socket::CallRequest(uint64_t bytes) {
		if(sock) {
			const uint64_t maxRequestSize = 1024*256;
			if(bytes > maxRequestSize) {
				bytes = maxRequestSize;
			}
			if(bytes == 0)
				bytes = 1;
			uint64_t currentSize = buffer.size();
			buffer.resize(currentSize+bytes, 0);
			fetchRequestSize = bytes;
			sock->async_read_some(boost::asio::buffer(
					&(buffer[currentSize]), bytes),
					std::bind(&Socket::FetchData,
						this,
						std::placeholders::_1,
						std::placeholders::_2));
		} else {
			printf("\n invalid socket");
		}
	}
	
	bool Socket::HasMessage() const {
		return !receivedMessages.empty();
	}
	
	float Socket::GetMessageCompletition(uint64_t& recvd, uint64_t& required) {
		recvd = buffer.size()-fetchRequestSize;
		required = 0;
		NumberBuffer n;
		uint64_t c = n.SetBytes(&buffer.front(), recvd);
		if(c == 0) {
			recvd = 0;
			required = 0;
			return 0.0f;
		}
		required = n.GetValue() + c;
		if(recvd > required)
			recvd = required;
		return 100.0 * (double)recvd / (double)required;
	}
	
	bool Socket::PopMessage(Message& message, int timeoutms) {
		if(ioContext) {
			ioContext->poll_one();
			int end = clock() + timeoutms;
			while(end>clock() && receivedMessages.empty()) {
				ioContext->poll_one();
				std::this_thread::yield();
				std::this_thread::yield();
			}
		}
		if(!receivedMessages.empty()) {
			receivedMessages.front().Swap(message);
			receivedMessages.pop();
			return true;
		}
		return false;
	}
	
	bool Socket::Valid() const {
		return sock!=NULL;
	}
	
	void Socket::Poll() {
		if(ioContext)
			ioContext->poll_one();
	}
	
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* Socket::GetSocket() {
		return sock;
	}
	
	
	
	Server::Server() {
		acceptor = NULL;
		sslContext = NULL;
		ioContext = NULL;
	}
	
	Server::~Server() {
		Close();
	}
	
	void Server::Open(const Endpoint& endpoint, const char* certChain,
				const char* privateKey, const char* dhFile) {
		Close();
		ioContext = new boost::asio::io_context();
		sslContext = new boost::asio::ssl::context(
				boost::asio::ssl::context::sslv23);
		sslContext->set_options(
				boost::asio::ssl::context::default_workarounds
				| boost::asio::ssl::context::no_sslv2
				| boost::asio::ssl::context::single_dh_use);
		sslContext->set_password_callback(
				std::bind(&Server::GetPassword, this));
		sslContext->use_certificate_chain_file(certChain);
		sslContext->use_private_key_file(privateKey,
				boost::asio::ssl::context::pem);
		sslContext->use_tmp_dh_file(dhFile);
		acceptor = new boost::asio::ip::tcp::acceptor(
				*ioContext,
				endpoint.TcpEndpoint());
		std::cout << "\n Endpoint: " << endpoint.TcpEndpoint();
	}
	
	void Server::Close() {
		if(acceptor) {
			acceptor->close();
			delete acceptor;
			acceptor = NULL;
		}
		if(sslContext)
			delete sslContext;
		sslContext = NULL;
		if(ioContext)
			delete ioContext;
		ioContext = NULL;
	}
	
	std::string Server::GetPassword() const {
		return "test";
	}
	
	bool Server::Listen(Socket* socket) {
		if(Valid() && socket) {
			socket->CreateEmpty(sslContext, ioContext);
			boost::system::error_code err;
			acceptor->accept(socket->sock->lowest_layer(), err);
			if(err) {
				socket->Close();
				return false;
			} else {
				socket->StartServerSide();
				ioContext->poll_one();
				return true;
			}
		}
		return false;
	}
	
	bool Server::Valid() const {
		return acceptor!=NULL && sslContext!=NULL;
	}
};

