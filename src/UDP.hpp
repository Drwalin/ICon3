
#ifndef UDP_HPP
#define UDP_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <map>
#include <unordered_map>
#include <queue>

#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>

#include "ASIO.hpp"

#ifdef ASIO_CPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/udp.hpp>

#else

#endif

namespace UDP {
	const uint32_t udpMessageSizeLimit = 1280;
	
	using GlobalEndpoint = Endpoint;
	
	class Endpoint {
	public:
		
		Endpoint();
		Endpoint(const GlobalEndpoint& endpoint);
		Endpoint(const Endpoint& endpoint);
		Endpoint(Endpoint&& endpoint) : ptr(endpoint.ptr) {}
		Endpoint(uint16_t port);
		~Endpoint();
		
		Endpoint& operator = (const Endpoint& other);
		bool operator < (const Endpoint& r) const;
		bool operator == (const Endpoint& r) const;
		bool operator != (const Endpoint& r) const;
		
		operator GlobalEndpoint() const;
		boost::asio::ip::udp::endpoint* ptr;
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

