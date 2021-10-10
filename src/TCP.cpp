/*
 *  This file is part of ICon3. Please see README for details.
 *  Copyright (C) 2020-2021 Marek Zalewski aka Drwalin
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
		if(socket)
			socket->close();
		SocketBase::Close();
	}
	
	bool Socket::Connect(const Endpoint& endpoint) {
		Close();
		socket = new ProtocolSocket(IoContext());
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
		socket = new ProtocolSocket(IoContext());
	}
	
	
	
	Server::Server() {
	}
	
	Server::~Server() {
	}
	
	
	void Server::Open(const Endpoint& endpoint) {
		ServerBase::Open(endpoint);
	}
	
	void Server::Close() {
		ServerBase::Close();
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
		return ServerBase::Valid();
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

