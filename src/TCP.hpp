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

#ifndef TCP_HPP
#define TCP_HPP

#include <string>
#include <vector>

#include "ASIO.hpp"
#include "Socket.hpp"

namespace tcp {
	
	using ProtocolSocket = boost::asio::ip::tcp::socket;
	using SocketBase = asio::Socket<ProtocolSocket>;
	
	class Socket : public asio::Socket<ProtocolSocket> {
	public:
		
		Socket();
		virtual ~Socket() override;
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& msg);
		bool TryPopMessage(Message& message, int timeoutms=-1);
		void GetMessageCompletition(uint64_t&recvd, uint64_t& required);
		ProtocolSocket* GetSocket();
		bool HasMessage() const;
		virtual void Close() override;
		
		bool Connect(const Endpoint& endpoint);
		
		void CreateEmptySocket();
	};
	
	
	
	using ServerBase = asio::Server<Socket>;
	
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

