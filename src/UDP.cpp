
#ifndef CPP_FILES_CPP
#define CPP_FILES_CPP
#endif

#include "UDP.hpp"

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

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/udp.hpp>

namespace udp {
	
	Endpoint::Endpoint() {
		ptr = new boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0);
	}
	
	Endpoint::Endpoint(const GlobalEndpoint& endpoint) {
		ptr = new boost::asio::ip::udp::endpoint(endpoint.UdpEndpoint());
	}
	
	Endpoint::Endpoint(const Endpoint& endpoint) {
		ptr = new boost::asio::ip::udp::endpoint(*endpoint.ptr);
	}
	
	Endpoint::Endpoint(uint16_t port) {
		ptr = new boost::asio::ip::udp::endpoint(
				boost::asio::ip::udp::v4(),
				port);
	}
	
	Endpoint::~Endpoint() {
		delete ptr;
	}
	
	Endpoint& Endpoint::operator = (const Endpoint& other) {
		delete ptr;
		ptr = new boost::asio::ip::udp::endpoint(*other.ptr);
		return *this;
	}
	bool Endpoint::operator < (const Endpoint& r) const {
		if(ptr && r.ptr)
			return (*ptr) < (*r.ptr);
		if(ptr==NULL && r.ptr==NULL)
			return false;
		return ptr==NULL && r.ptr;
	}
	bool Endpoint::operator == (const Endpoint& r) const {
		if(ptr && r.ptr)
			return (*ptr) == (*r.ptr);
		if(ptr==NULL && r.ptr==NULL)
			return true;
		return false;
	}
	bool Endpoint::operator != (const Endpoint& r) const {
		if(ptr && r.ptr)
			return (*ptr) != (*r.ptr);
		if(ptr==NULL && r.ptr==NULL)
			return false;
		return true;
	}
	
	Endpoint::operator GlobalEndpoint() const {
		return GlobalEndpoint(*ptr);
	}
	
	
	
	Socket::Socket() {
		nextEmptyId = 1;
		sock = NULL;
	}
	
	Socket::~Socket() {
		Close();
	}
	
	void Socket::Open(const GlobalEndpoint& endpoint) {
		Close();
		Endpoint end(endpoint);
		sock = new boost::asio::ip::udp::socket(IoService(), *end.ptr);
	}
	
	void Socket::Close() {
		if(sock) {
			sock->close();
			delete sock;
			sock = NULL;
		}
		endpointId.clear();
		received.clear();
		nextEmptyId = 1;
	}
	
	
	bool Socket::Send(const std::vector<uint8_t>& buffer,
			const Endpoint& endpoint) {
		if(sock!=NULL && buffer.size()<=udpMessageSizeLimit) {
			boost::system::error_code err;
			sock->send_to(boost::asio::buffer(buffer), *endpoint.ptr, 0, err);
			if(err)
				return false;
			return true;
		}
		return false;
	}
	bool Socket::Send(const std::vector<uint8_t>& buffer, uint64_t id) {
		if(endpointId.has(id)) {
			const Endpoint& endpoint = endpointId[id];
			return Send(buffer, endpoint);
		}
		return false;
	}
	bool Socket::Send(const std::vector<uint8_t>& buffer,
			const GlobalEndpoint& endpoint) {
		Endpoint end(endpoint);
		return Send(buffer, end);
	}
	bool Socket::Send(const Message& message, const GlobalEndpoint& endpoint) {
		std::vector<uint8_t> buffer;
		CreateOptimalBuffer(message, buffer);
		return Send(buffer, endpoint);
	}
	bool Socket::Send(const Message& message, const Endpoint& endpoint) {
		std::vector<uint8_t> buffer;
		CreateOptimalBuffer(message, buffer);
		return Send(buffer, endpoint);
	}
	bool Socket::Send(const Message& message, uint64_t id) {
		if(endpointId.has(id)) {
			const Endpoint& endpoint = endpointId[id];
			return Send(message, endpoint);
		}
		return false;
	}
	
	
	void Socket::FetchData() {
		if(sock) {
			Endpoint endpoint;
			boost::system::error_code err;
			while(sock->available()>0) {
				size_t recvd = sock->receive_from(
						boost::asio::buffer(
							recvTempBuffer,
							udpMessageSizeLimit),
						*endpoint.ptr,
						0, err);
				if(!err) {
					uint64_t id = GetId(endpoint);
					std::queue<Message>& msgs = received[id];
					msgs.emplace();
					size_t readed = TryReadMessageFromBuffer(
						msgs.back(), recvTempBuffer, recvd);
				} else {
					fprintf(stderr, "\n Error while receiving: %i", err);
				}
			}
		}
	}
	
	bool Socket::HasAnyMessage() const {
		for(const auto& it : received)
			return !it.second.empty();
		return false;
	}
	
	bool Socket::HasMessage(uint64_t id) {
		if(id == 0)
			return HasAnyMessage();
		auto it = received.find(id);
		if(it != received.end())
			return !it->second.empty();
		return false;
	}
	
	
	bool Socket::InternalPopMessage(Message& message,
			uint64_t& id, // when id=0 => pop any message
			int timeoutms) {
		FetchData();
		if(timeoutms > 0) {
			int end = clock() + timeoutms;
			while(!HasMessage(id) && end>clock()) {
				std::this_thread::yield();
				FetchData();
				std::this_thread::yield();
			}
		}
		if(HasMessage(id)) {
			if(id == 0) {
				auto it = received.begin();
				for(; it!=received.end(); ++it) {
					if(it->second.size() > 0) {
						id = it->first;
						message.Swap(it->second.front());
						it->second.pop();
						if(it->second.empty())
							received.erase(it);
						return true;
					} else
						it = received.erase(it);
				}
			} else {
				auto it = received.find(id);
				if(it != received.end()) {
					message.Swap(it->second.front());
					it->second.pop();
					if(it->second.empty())
						received.erase(it);
					return true;
				}
			}
		}
		return false;
	}
	
	bool Socket::PopAnyMessage(Message& message,
			uint64_t& id, 
			int timeoutms) {
		id = 0;
		return InternalPopMessage(message, id, timeoutms);
	}
	bool Socket::PopMessage(Message& message,
			const uint64_t id,
			int timeoutms) {
		uint64_t _id = id;
		return InternalPopMessage(message, _id, timeoutms);
	}
	
	
	void Socket::CloseEndpoint(const GlobalEndpoint& endpoint) {
		Endpoint end(endpoint);
		received.erase(endpointId[end]);
		endpointId.erase(end);
	}
	
	void Socket::CloseEndpoint(const Endpoint& endpoint) {
		received.erase(endpointId[endpoint]);
		endpointId.erase(endpoint);
	}
	
	void Socket::CloseEndpoint(const uint64_t id) {
		received.erase(id);
		endpointId.erase(id);
	}
	
	
	uint64_t Socket::PopNextEmptyId() {
		while(true) {
			uint64_t id = ++nextEmptyId;
			if(id!=0 && id!=(~(uint64_t)(0)))
				if(endpointId.has(id) == false)
					return id;
		}
		DEBUG("An unknown error appeared in system!");
		return 0;
	}
	
	uint64_t Socket::GetId(const GlobalEndpoint& endpoint) {
		Endpoint end(endpoint);
		if(endpointId.has(end))
			return endpointId[end];
		else {
			uint64_t id = PopNextEmptyId();
			endpointId.insert(end, id);
			return id;
		}
		return 0;
	}
	
	GlobalEndpoint Socket::GetEndpoint(const uint64_t id) const {
		if(endpointId.has(id))
			return (GlobalEndpoint)endpointId[id];
		return GlobalEndpoint();
	}
	
	
	
	Connection::Connection(std::shared_ptr<Socket> socket,
			const GlobalEndpoint& endpoint) :
		socket(socket),
		globalEndpoint(endpoint),
		endpoint(endpoint) {
		id = socket->GetId(this->endpoint);
	}
	
	Connection::~Connection() {
		socket->CloseEndpoint(id);
	}
	
	bool Connection::Send(const std::vector<uint8_t>& buffer) {
		return socket->Send(buffer, endpoint);
	}
	
	bool Connection::Send(const Message& message) {
		return socket->Send(message, endpoint);
	}
	
	bool Connection::HasMessage() const {
		return socket->HasMessage(id);
	}
	
	bool Connection::PopMessage(Message& message, int timeoutms) {
		return socket->PopMessage(message, id, timeoutms);
	}
	
	bool Connection::operator == (const Connection& r) const {
		return id==r.id && socket==r.socket;
	}
	
	bool Connection::operator != (const Connection& r) const {
		return id!=r.id || socket!=r.socket;
	}
	
	bool Connection::operator < (const Connection& r) const {
		if((size_t)socket.get() < (size_t)r.socket.get())
			return true;
		if((size_t)socket.get() > (size_t)r.socket.get())
			return false;
		return id < r.id;
	}
};

