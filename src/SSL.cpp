/*
 *  This file is part of ICon3. Please see README for details.
 *  Copyright (C) 2020 Marek Zalewski aka Drwalin
 *
 *  ICon3 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CPP_FILES_CPP
#define CPP_FILES_CPP
#endif

#include "Socket.cpp"
#include "SSL.hpp"

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
#include <boost/asio/ssl.hpp>

namespace ssl {
	
	Socket::Socket() {
	}
	
	Socket::~Socket() {
	}
	
	bool Socket::Send(const std::vector<uint8_t>& buffer) {
		return SocketBase::Send(buffer);
	}
	bool Socket::Send(const Message& msg) {
		return SocketBase::Send(msg);
	}
	bool Socket::TryPopMessage(Message& message, int timeoutms) {
		return SocketBase::TryPopMessage(message, timeoutms);
	}
	void Socket::GetMessageCompletition(uint64_t&recvd, uint64_t& required) {
		SocketBase::GetMessageCompletition(recvd, required);
	}
	ProtocolSocket* Socket::GetSocket() {
		return SocketBase::GetSocket();
	}
	bool Socket::HasMessage() const {
		return SocketBase::HasMessage();
	}
	void Socket::Close() {
		SocketBase::Close();
	}
	
	bool Socket::Connect(const Endpoint& endpoint, const char* certificateFile) {
		Close();
		boost::asio::ssl::context sslContext(boost::asio::ssl::context::sslv23);
		sslContext.load_verify_file(certificateFile);
		socket = new ProtocolSocket(IoContext(), sslContext);
		boost::system::error_code err;
		socket->lowest_layer().connect(endpoint.TcpEndpoint(), err);
		if(err) {
			fprintf(stderr, "\n Failed to connect: %s", err.message());
			fflush(stderr);
			delete socket;
			socket = NULL;
			return false;
		}
		socket->handshake(boost::asio::ssl::stream_base::client, err);
		if(err) {
			fprintf(stderr, "\n Failed to handshake: %s", err.message());
			fflush(stderr);
			delete socket;
			socket = NULL;
			return false;
		}
		FinalizeConnecting();
		return true;
	}
	
	void Socket::CreateEmptySocket(boost::asio::ssl::context* sslContext) {
		Close();
		socket = new ProtocolSocket(IoContext(), *sslContext);
	}
	
	bool Socket::StartServerSide() {
		if(Valid()) {
			boost::system::error_code err;
			socket->handshake(boost::asio::ssl::stream_base::server, err);
			if(err) {
				Close();
				printf("\n Failed to make handshake: %s", err.message());
				return false;
			}
			return true;
		}
		return false;
	}
	
	
	
	Server::Server() {
		sslContext = NULL;
	}
	
	Server::~Server() {
	}
	
	
	void Server::Open(const Endpoint& endpoint, const char* certChainFile,
				const char* privateKeyFile, const char* dhFile) {
		ServerBase::Open(endpoint);
		sslContext = new boost::asio::ssl::context(
				boost::asio::ssl::context::sslv23);
		sslContext->set_options(
				boost::asio::ssl::context::default_workarounds
				| boost::asio::ssl::context::no_sslv2
				| boost::asio::ssl::context::single_dh_use);
		sslContext->set_password_callback(std::bind([]()->std::string{return "test";}));
		sslContext->use_certificate_chain_file(certChainFile);
		sslContext->use_private_key_file(privateKeyFile,
				boost::asio::ssl::context::pem);
		sslContext->use_tmp_dh_file(dhFile);
	}
	
	void Server::Close() {
		ServerBase::Close();
		if(sslContext) {
			delete sslContext;
			sslContext = NULL;
		}
	}
	
	
	bool Server::IsListening() {
		return ServerBase::IsListening();
	}
	
	bool Server::HasNewSocket() const {
		return ServerBase::HasNewSocket();
	}
	
	Socket* Server::TryGetNewSocket(int timeoutms) {
		return ServerBase::TryGetNewSocket(timeoutms);
	}
	
	Socket* Server::GetNewSocket() {
		return ServerBase::GetNewSocket();
	}
	
	void Server::StartListening() {
		ServerBase::StartListening();
	}
	
	
	bool Server::Valid() const {
		return ServerBase::Valid() && (sslContext!=NULL);
	}
	
	
	Socket* Server::CreateEmptyAcceptingSocket() {
		if(Valid()) {
			Socket* socket = new Socket();
			socket->CreateEmptySocket(sslContext);
			return socket;
		}
		return NULL;
	}
	
	bool Server::Accept(Socket* socket) {
		if(socket) {
			socket->StartServerSide();
			socket->FinalizeConnecting();
			return true;
		}
		return false;
	}
};

