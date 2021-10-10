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

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "ASIO.hpp"

#include <vector>
#include <queue>

namespace asio {
	
	const static uint64_t maxSinglePacketSize = 64*1024;
	
	template<typename T>
	class Socket {
	public:
		
		Socket();
		virtual ~Socket();
		Socket(Socket&&) = delete;
		Socket(const Socket&) = delete;
		Socket& operator =(const Socket&) = delete;
		
		virtual void Close();
		
		const Endpoint& GetEndpoint() const;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		
		bool HasMessage() const;
		void GetMessageCompletition(uint64_t&recvd, uint64_t& required);
		void GetBufferMessageCompletition(uint64_t&recvd, uint64_t& required);
		bool TryPopMessage(Message& message, int timeoutms=-1);
		
		T* GetSocket();
		
		bool Valid() const;
		
		template<typename T2>
		friend class Server;
		
		virtual bool FinalizeConnecting();
		
	protected:
		
#ifdef SOCKET_CPP
		void FetchData(const boost::system::error_code& err, size_t length);
#endif
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
		virtual ~Server();
		
		void Open(const Endpoint& endpoint);
		virtual void Close();
		
		bool IsListening();
		bool HasNewSocket() const;
		T* TryGetNewSocket(int timeoutms=-1);
		T* GetNewSocket();
		void StartListening();
		
		virtual bool Valid() const;
		
		
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

