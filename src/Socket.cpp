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

#ifndef SOCKET_CPP
#define SOCKET_CPP

#ifndef CPP_FILES_CPP
#define CPP_FILES_CPP
#endif

#include "Socket.hpp"

#include <thread>

namespace asio {
	
	template<typename T>
	Socket<T>::Socket() {
		socket = NULL;
		fetchRequestSize = 0;
	}
	
	template<typename T>
	Socket<T>::~Socket() {
		Close();
	}
	
	template<typename T>
	void Socket<T>::Close() {
		endpoint = Endpoint();
		if(socket) {
			delete socket;
			socket = NULL;
		}
		buffer.clear();
		buffer.shrink_to_fit();
		fetchRequestSize = 0;
		while(!receivedMessages.empty())
			receivedMessages.pop();
	}
	
	
	template<typename T>
	const Endpoint& Socket<T>::GetEndpoint() const {
		return endpoint;
	}
	
	
	template<typename T>
	bool Socket<T>::Send(const std::vector<uint8_t>& buffer) {
		if(Valid()) {
			boost::system::error_code err;
			for(uint64_t i=0; i<buffer.size();) {
				uint64_t toWrite = std::min<uint64_t>(
						buffer.size()-i,
						maxSinglePacketSize);
				IoContextPollOne();
				uint64_t written = socket->write_some(
						boost::asio::buffer(
							&(buffer[i]),
							toWrite),
						err);
				
				i += written;
				if(err) {
					fprintf(stderr, "\n fault send: %llu / %llu,  error: %s", i,
							buffer.size(), err.message());
					return false;
				} else if(written == 0) {
					fprintf(stderr, "\n fault send (0): %llu / %llu", i,
							buffer.size());
				}
			}
			return true;
		}
		return false;
	}
	
	template<typename T>
	bool Socket<T>::Send(const Message& msg) {
		if(Valid()) {
			std::vector<uint8_t> buffer;
			CreateOptimalBuffer(msg, buffer);
			return Send(buffer);
		}
		return false;
	}
	
	
	template<typename T>
	bool Socket<T>::HasMessage() const {
		return !receivedMessages.empty();
	}
	
	template<typename T>
	void Socket<T>::GetMessageCompletition(uint64_t&recvd, uint64_t& required) {
		if(HasMessage()) {
			required = recvd = receivedMessages.front().title.size()+1
				+ receivedMessages.front().data.size();
		} else {
			GetBufferMessageCompletition(recvd, required);
		}
	}
	
	template<typename T>
	void Socket<T>::GetBufferMessageCompletition(uint64_t&recvd,
			uint64_t& required) {
		recvd = buffer.size()-fetchRequestSize;
		required = 0;
		NumberBuffer n;
		uint64_t c = n.SetBytes(&buffer.front(), recvd);
		if(c == 0) {
			recvd = 0;
			required = 0;
			return;
		}
		required = n.GetValue() + c;
		if(recvd > required)
			recvd = required;
	}
	
	template<typename T>
	bool Socket<T>::TryPopMessage(Message& message, int timeoutms) {
		IoContextPollOne();
		int end = clock() + timeoutms;
		while(end>clock() && receivedMessages.empty() && Valid()) {
			std::this_thread::yield();
			IoContextPollOne();
			std::this_thread::yield();
		}
		if(!receivedMessages.empty()) {
			receivedMessages.front().Swap(message);
			receivedMessages.pop();
			return true;
		}
		return false;
	}
	
	
	template<typename T>
	T* Socket<T>::GetSocket() {
		return socket;
	}
	
	
	template<typename T>
	bool Socket<T>::Valid() const {
		return socket!=NULL;
	}
	
	
	template<typename T>
	bool Socket<T>::FinalizeConnecting() {
		if(Valid()) {
			this->endpoint = Endpoint(socket->lowest_layer().remote_endpoint());
			RequestDataFetch(1);
			return true;
		}
		return false;
	}
	
	
	template<typename T>
	void Socket<T>::FetchData(const boost::system::error_code& err,
			size_t length) {
		if(err) {
			fprintf(stderr, "\n Error occured while fetching data: %s", err.message());
		} else if(Valid()) {
			if(fetchRequestSize != length)
				buffer.resize(buffer.size()-(fetchRequestSize-length));
			fetchRequestSize = 0;
			if(!err) {
				uint64_t recvd=0, required=0;
				GetBufferMessageCompletition(recvd, required);
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
					RequestDataFetch(required-recvd);
					return;
				}
			}
			RequestDataFetch(1);
		} else {
			DEBUG("Socket is invalid while reading fetching data!");
		}
	}
	
	template<typename T>
	void Socket<T>::RequestDataFetch(uint64_t bytes) {
		if(Valid()) {
			if(bytes > maxSinglePacketSize) {
				bytes = maxSinglePacketSize;
			}
			if(bytes == 0)
				bytes = 1;
			uint64_t currentSize = buffer.size();
			buffer.resize(currentSize+bytes);
			fetchRequestSize = bytes;
			socket->async_read_some(boost::asio::buffer(
					&(buffer[currentSize]), bytes),
					std::bind(&Socket::FetchData,
						this,
						std::placeholders::_1,
						std::placeholders::_2));
		} else {
			DEBUG("\n Socket is invalid while requesting data fetch!");
		}
	}
	
	
	
	
	
	template<typename T>
	Server<T>::Server() {
		acceptor = NULL;
		currentAcceptingSocket = NULL;
	}
	
	template<typename T>
	Server<T>::~Server() {
		Close();
	}
	
	
	template<typename T>
	void Server<T>::Open(const Endpoint& endpoint) {
		Close();
		acceptor = new boost::asio::ip::tcp::acceptor(
				IoContext(),
				endpoint.TcpEndpoint());
	}
	
	template<typename T>
	void Server<T>::Close() {
		if(acceptor) {
			acceptor->close();
			delete acceptor;
			acceptor = NULL;
		}
	}
	
	
	template<typename T>
	bool Server<T>::IsListening() {
		return Valid() && currentAcceptingSocket!=NULL;
	}
	
	template<typename T>
	bool Server<T>::HasNewSocket() const {
		if(Valid()) {
			if(newSockets.empty())
				IoContextPollOne();
			return !newSockets.empty();
		}
		return false;
	}
	
	template<typename T>
	T* Server<T>::TryGetNewSocket(int timeoutms) {
		IoContextPollOne();
		int end = clock() + timeoutms;
		while(end>clock() && newSockets.empty() && Valid()) {
			std::this_thread::yield();
			IoContextPollOne();
			std::this_thread::yield();
		}
		if(!newSockets.empty()) {
			T* ret = newSockets.front();
			newSockets.pop();
			return ret;
		}
		return NULL;
	}
	
	template<typename T>
	T* Server<T>::GetNewSocket() {
		while(Valid()) {
			T* socket = TryGetNewSocket(1000);
			if(socket)
				return socket;
		}
		return NULL;
	}
	
	template<typename T>
	void Server<T>::StartListening() {
		if(!IsListening()) {
			currentAcceptingSocket = CreateEmptyAcceptingSocket();
			acceptor->async_accept(
					currentAcceptingSocket->GetSocket()->lowest_layer(),
					std::bind(&Server<T>::DoAccept, this,
						std::placeholders::_1));
		}
	}
	
	
	template<typename T>
	bool Server<T>::Valid() const {
		return acceptor!=NULL;
	}
	
	
	template<typename T>
	void Server<T>::DoAccept(const boost::system::error_code& err) {
		if(err) {
			fprintf(stderr, "\n Error while accepting new socket");
			delete currentAcceptingSocket;
			currentAcceptingSocket = NULL;
		} else {
			if(Accept(currentAcceptingSocket)) {
				newSockets.emplace(currentAcceptingSocket);
				currentAcceptingSocket = NULL;
			} else {
				delete currentAcceptingSocket;
				currentAcceptingSocket = NULL;
			}
			StartListening();
		}
	}
	
	template<typename T>
	bool Server<T>::Accept(T* socket) {
		return socket!=NULL;
	}
	
};

#endif

