
#ifndef ASIO_HPP
#define ASIO_HPP

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include <cinttypes>

#ifdef CPP_FILES_CPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ssl.hpp>

extern "C" boost::asio::io_service *io_service;
boost::asio::io_service& IoService();

#else

namespace boost {
	namespace asio {
		class io_context;
		class io_service;
		namespace ip {
			namespace tcp {
				class socket;
				class acceptor;
				class endpoint;
			};
			namespace udp {
				class socket;
				class endpoint;
			};
		};
		namespace ssl {
			template<typename T>
			class stream;
			class context;
		};
	};
};

#endif

#define DEBUG(x) {fprintf(stderr,"\n %s : %s:%i",x,__FILE__,__LINE__); fflush(stderr);}

class BasicSocket {
public:
};

class Endpoint {
public:
	
	Endpoint();
	Endpoint(const char* ip, uint16_t port);
	Endpoint(const char* ipport);
	
#ifdef CPP_FILES_CPP
	Endpoint(boost::asio::ip::tcp::endpoint tcp);
	Endpoint(boost::asio::ip::udp::endpoint udp);
	boost::asio::ip::udp::endpoint UdpEndpoint() const;
	boost::asio::ip::tcp::endpoint TcpEndpoint() const;
#endif
	
	bool operator == (const Endpoint& other) const;
	bool operator < (const Endpoint& other) const;
	operator bool() const;
	
	std::string ToString() const;
	
	std::string ip;
	uint16_t port;
};

class NumberBuffer {
public:
	
	struct SIZE__ {uint8_t d[18];};
	
	NumberBuffer();
	NumberBuffer(uint64_t val);
	NumberBuffer(uint8_t* data, size_t max);
	NumberBuffer(const NumberBuffer& other);
	NumberBuffer(NumberBuffer&& other);
	
	NumberBuffer& operator = (const NumberBuffer& other);
	
	size_t GetOccupiedBytes() const;
	const uint8_t* GetData() const;
	uint64_t GetValue() const;
	
	size_t SetValue(uint64_t val);
	size_t SetBytes(const uint8_t* data, size_t max);
	void Print();
	
private:
	
	uint64_t value;
	uint8_t bytes[10];
};

class Message {
public:
	
	Message();
	Message(const Message& other);
	Message(Message&& other);
	Message(const std::string& title, const std::vector<uint8_t>& data);
	Message(const std::string& title, void* data, int dataLength);
	Message(const std::string& title);
	Message(const std::string& title, const std::string& data);
	
	Message& operator = (const Message& other);
	
	void Swap(Message& other);
	
	std::string title;
	std::vector<uint8_t> data;
};

template<typename T1, typename T2>
class BiMap {
public:
	
	BiMap(){}
	BiMap(const BiMap& other) :
		t1t2(other.t1t2),
		t2t1(other.t2t1),
		temp1(other.temp1),
		temp2(other.temp2) {
	}
	BiMap(BiMap&& other) :
		t1t2(other.t1t2),
		t2t1(other.t2t1),
		temp1(other.temp1),
		temp2(other.temp2) {
	}
	~BiMap(){}
	
	void insert(const T1& t1, const T2& t2) {
		erase(t1);
		erase(t2);
		t1t2[t1] = t2;
		t2t1[t2] = t1;
	}
	
	void erase(const T1& t1) {
		auto it = t1t2.find(t1);
		if(it != t1t2.end()) {
			t2t1.erase(it->second);
			t1t2.erase(it);
		}
	}
	
	void erase(const T2& t2) {
		auto it = t2t1.find(t2);
		if(it != t2t1.end()) {
			t1t2.erase(it->second);
			t2t1.erase(it);
		}
	}
	
	const T1& getA(const T2& t2) const {
		auto it = t2t1.find(t2);
		if(it != t2t1.end()) {
			return it->second;
		}
		return temp1;
	}
	
	const T2& getB(const T1& t1) const {
		auto it = t1t2.find(t1);
		if(it != t1t2.end()) {
			return it->second;
		}
		return temp2;
	}
	
	const T1& operator [] (const T2& t2) const {
		return getA(t2);
	}
	
	const T2& operator [] (const T1& t1) const {
		return getB(t1);
	}
	
	bool has(const T2& t2) const {
		auto it = t2t1.find(t2);
		return it != t2t1.end();
	}
	
	bool has(const T1& t1) const {
		auto it = t1t2.find(t1);
		return it != t1t2.end();
	}
	
	void clear() {
		t1t2.clear();
		t2t1.clear();
	}
	
	T1 temp1;
	T2 temp2;
	std::map<T1, T2> t1t2;
	std::map<T2, T1> t2t1;
};

void CreateOptimalBuffer(const Message& msg, std::vector<uint8_t>& buffer);
bool CanReadFullMessage(const uint8_t* buffer, uint64_t bufferSize);
bool CanReadFullMessage(const std::vector<uint8_t>& buffer);
uint64_t TryReadMessageFromBuffer(Message& msg,
		uint8_t* buffer,
		uint64_t bufferSize);
uint64_t TryReadMessageFromBuffer(Message& msg, std::vector<uint8_t>& buffer);

#endif

