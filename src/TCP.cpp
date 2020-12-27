
#ifndef CPP_FILES_CPP
#define CPP_FILES_CPP
#endif

#include "Socket.cpp"
#include "TCP.hpp"

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>

#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace tcp {
	
	Socket::Socket() {
	}
	
	Socket::~Socket() {
	}
	
	bool Socket::Send(const std::vector<uint8_t>& buffer) {
		return asio::Socket<boost::asio::ip::tcp::socket>::Send(buffer);
	}
	bool Socket::Send(const Message& msg) {
		return asio::Socket<boost::asio::ip::tcp::socket>::Send(msg);
	}
	bool Socket::TryPopMessage(Message& message, int timeoutms) {
		return asio::Socket<boost::asio::ip::tcp::socket>::TryPopMessage(message, timeoutms);
	}
	void Socket::GetMessageCompletition(uint64_t&recvd, uint64_t& required) {
		return asio::Socket<boost::asio::ip::tcp::socket>::GetMessageCompletition(recvd, required);
	}
	boost::asio::ip::tcp::socket* Socket::GetSocket() {
		return asio::Socket<boost::asio::ip::tcp::socket>::GetSocket();
	}
	bool Socket::HasMessage() const {
		return asio::Socket<boost::asio::ip::tcp::socket>::HasMessage();
	}
	void Socket::Close() {
		return asio::Socket<boost::asio::ip::tcp::socket>::Close();
	}
	
	bool Socket::Connect(const Endpoint& endpoint) {
		Close();
		socket = new boost::asio::ip::tcp::socket(IoContext());
		boost::system::error_code err;
		socket->connect(endpoint.TcpEndpoint(), err);
		if(err) {
			delete socket;
			socket = NULL;
			return false;
		}
		FinalizeConnecting();
		return true;
	}
	
	void Socket::CreateEmptySocket() {
		Close();
		socket = new boost::asio::ip::tcp::socket(IoContext());
	}
	
	
	
	Server::Server() {
	}
	
	Server::~Server() {
	}
	
	
	void Server::Open(const Endpoint& endpoint) {
		return asio::Server<Socket>::Open(endpoint);
	}
	
	void Server::Close() {
		return asio::Server<Socket>::Close();
	}
	
	
	bool Server::IsListening() {
		return asio::Server<Socket>::IsListening();
	}
	
	bool Server::HasNewSocket() const {
		return asio::Server<Socket>::HasNewSocket();
	}
	
	Socket* Server::TryGetNewSocket(int timeoutms) {
		return asio::Server<Socket>::TryGetNewSocket(timeoutms);
	}
	
	Socket* Server::GetNewSocket() {
		return asio::Server<Socket>::GetNewSocket();
	}
	
	void Server::StartListening() {
		return asio::Server<Socket>::StartListening();
	}
	
	
	bool Server::Valid() const {
		return asio::Server<Socket>::Valid();
	}
	
	
	Socket* Server::CreateEmptyAcceptingSocket() {
		Socket* socket = new Socket();
		socket->CreateEmptySocket();
		return socket;
	}
	
	bool Server::Accept(Socket* socket) {
		if(socket) {
			socket->FinalizeConnecting();
			return true;
		}
		return false;
	}
};

