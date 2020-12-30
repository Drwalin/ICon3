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

#ifndef UDP_HPP
#define UDP_HPP

#include "ASIO.hpp"

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <map>
#include <unordered_map>
#include <queue>

namespace udp {
	
	const uint32_t udpMessageSizeLimit = 1280;
	
	using GlobalEndpoint = Endpoint;
	
	class Endpoint {
	public:
		
		Endpoint(bool empty);
		Endpoint(bool ref, boost::asio::ip::udp::endpoint* ptr);
		Endpoint();
		Endpoint(const GlobalEndpoint& endpoint);
		Endpoint(const Endpoint& endpoint);
		Endpoint(Endpoint&& endpoint) : ptr(endpoint.ptr), ref(endpoint.ref) {}
		Endpoint(uint16_t port);
		~Endpoint();
		
		static Endpoint MakeEmpty();
		static Endpoint Make(boost::asio::ip::udp::endpoint* ptr);
		
		Endpoint& operator = (const Endpoint& other);
		bool operator < (const Endpoint& r) const;
		bool operator == (const Endpoint& r) const;
		bool operator != (const Endpoint& r) const;
		
		operator GlobalEndpoint() const;
		
		boost::asio::ip::udp::endpoint* ptr;
	private:
		bool ref;
	};
	
	
	
	class Socket {
	public:
		
		Socket();
		~Socket();
		
		void Open(const GlobalEndpoint& endpoint);
		void Close();
		
		bool Send(const std::vector<uint8_t>& buffer, uint64_t id);
		bool Send(const std::vector<uint8_t>& buffer, const Endpoint& endpoint);
		bool Send(const std::vector<uint8_t>& buffer,
				const GlobalEndpoint& endpoint);
		bool Send(const Message& message, const GlobalEndpoint& endpoint);
		bool Send(const Message& message, const Endpoint& endpoint);
		bool Send(const Message& message, uint64_t id);
		
		void FetchData();
		bool HasAnyMessage() const;
		bool HasMessage(uint64_t id);
		
		bool InternalPopMessage(Message& message,
				uint64_t& id, // when id=0 => pop any message
				int timeoutms=-1);
		bool PopAnyMessage(Message& message, uint64_t& id, int timeoutms=-1);
		bool PopMessage(Message& message, const uint64_t id, int timeoutms=-1);
		
		void CloseEndpoint(const GlobalEndpoint& endpoint);
		void CloseEndpoint(const Endpoint& endpoint);
		void CloseEndpoint(const uint64_t id);
		
		uint64_t PopNextEmptyId();
		uint64_t GetId(const GlobalEndpoint& endpoint);
		GlobalEndpoint GetEndpoint(const uint64_t id) const;
		
	private:
		
		uint64_t nextEmptyId;
		boost::asio::ip::udp::socket* sock;
		std::unordered_map<uint64_t, std::queue<Message>> received;
		BiMap<Endpoint, uint64_t> endpointId;
		uint8_t recvTempBuffer[udpMessageSizeLimit];
	};
	
	
	
	class Connection {
	public:
		
		Connection(std::shared_ptr<Socket> socket,
				const GlobalEndpoint& endpoint);
		~Connection();
		
		bool Send(const std::vector<uint8_t>& buffer);
		bool Send(const Message& message);
		
		bool HasMessage() const;
		bool PopMessage(Message& message, int timeoutms=-1);
		
		inline bool operator == (const Connection& r) const;
		inline bool operator != (const Connection& r) const;
		inline bool operator < (const Connection& r) const;
		
		std::shared_ptr<Socket> socket;
		Endpoint endpoint;
		GlobalEndpoint globalEndpoint;
		uint64_t id;
	};
};

#endif

