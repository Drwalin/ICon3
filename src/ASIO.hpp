
#ifndef ASIO_HPP
#define ASIO_HPP

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include <cinttypes>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifdef ASIO_CPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

boost::asio::io_service *io_service = NULL;
boost::asio::io_service& IoService() {
	if(io_service == NULL) {
		io_service = new boost::asio::io_service;
		io_service->run();
	}
	return *io_service;
}

#else

namespace boost {
	namespace asio {
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
	};
};


#endif

#define DEBUG(x) {fprintf(stderr,"\n %s : %s:%i",x,__FILE__,__LINE__); fflush(stderr);}

class Endpoint {
public:
	
	Endpoint() : port(0) {}
	Endpoint(const char* ip, uint16_t port) :
		ip(ip), port(port) {}
	Endpoint(const char* ipport) {
		int i=0;
		for(; ipport[i] && ipport[i]!=':'; ++i) {
		}
		
		if(ipport[i] == ':') {
			ip = std::string(ipport, i);
			int p = atoi(ipport+i+1);
			if(p>=(1<<16) || p<0) {
				ip = "";
				port = 0;
			} else
				port = p;
		} else {
			port = 0;
		}
	}
	
	
	inline operator bool() const {
		return ip!="";
	}
	
#ifdef ASIO_CPP
	Endpoint(boost::asio::ip::tcp::endpoint tcp) {
		ip = tcp.address().to_string();
		port = tcp.port();
	}
	Endpoint(boost::asio::ip::udp::endpoint udp) {
		ip = udp.address().to_string();
		port = udp.port();
	}
	
	inline boost::asio::ip::udp::endpoint UdpEndpoint() const {
		if(*this)
			return boost::asio::ip::udp::endpoint(
					boost::asio::ip::address::from_string(ip.c_str()),
					port);
		else
			return boost::asio::ip::udp::endpoint(
					boost::asio::ip::udp::v4(),
					port);
	}
	inline boost::asio::ip::tcp::endpoint TcpEndpoint() const {
		if(*this) {
			return boost::asio::ip::tcp::endpoint(
					boost::asio::ip::address::from_string(ip.c_str()),
					port);
		} else {
			return boost::asio::ip::tcp::endpoint(
					boost::asio::ip::tcp::v4(),
					port);
		}
	}
#endif
	
	inline bool operator == (const Endpoint& other) const {
		return ip==other.ip && port==other.port;
	}
	
	inline bool operator < (const Endpoint& other) const {
		return ip<other.ip || (ip==other.ip && port<other.port);
	}
	
	inline std::string ToString() const {
		return ip + ":" + std::to_string(port);
	}
	
	
	std::string ip;
	uint16_t port;
};

class NumberBuffer {
public:
	struct SIZE__ {uint8_t d[18];};
	NumberBuffer() {
		value = 0;
		*((uint64_t*)bytes) = 0;
		bytes[8] = 0;
		bytes[9] = 0;
	}
	NumberBuffer(uint64_t val) {
		SetValue(val);
	}
	NumberBuffer(uint8_t* data, size_t max) {
		SetBytes(data, max);
	}
	NumberBuffer(const NumberBuffer& other) {
		*(SIZE__*)this = *(SIZE__*)&other;
	}
	NumberBuffer(NumberBuffer&& other) {
		*(SIZE__*)this = *(SIZE__*)&other;
	}
	NumberBuffer& operator = (const NumberBuffer& other) {
		*(SIZE__*)this = *(SIZE__*)&other;
		return *this;
	}
	
	inline size_t GetOccupiedBytes() const {
		int i=0;
		for(; i<10 && bytes[i]&128; ++i) {
		}
		return i+1;
	}
	inline const uint8_t* GetData() const {
		return bytes;
	}
	inline uint64_t GetValue() const {
		return value;
	}
	
	inline size_t SetValue(uint64_t val) {
		value = val;
		for(int i=0; i<10; ++i)
			bytes[i] = 0;
		
		size_t i = 0;
		if(val == 0) {
			bytes[0] = 0;
			return 1;
		}
		while(val > 0) {
			if(val > 127) {
				bytes[i] = (uint8_t)(val&127) | 128;
			} else {
				bytes[i] = (uint8_t)(val&127);
				break;
			}
			val >>= 7;
			++i;
		}
		return i+1;
	}
	inline size_t SetBytes(const uint8_t* data, size_t max) {
		value = 0;
		if(max > 10)
			max = 10;
		for(int i=0; i<max; ++i)
			bytes[i] = data[i];
		size_t i = 0;
		for(;; ++i) {
			if(i >= max)
				return 0;
			value |= ((uint64_t)bytes[i]&0x7F) << (i*7);
			if((bytes[i]&128) == 0)
				break;
		}
		return i+1;
	}
	
	inline void Print() {
		for(int i=0; i<10; ++i) {
			uint8_t b = bytes[i];
			if(i>0)
				printf(" ");
			for(int i=7;i>=0; --i)
				printf((b&(1<<i)) ? "1" : "0");
			if((b&128) == 0)
				break;
		}
	}
	
private:
	
	uint64_t value;
	uint8_t bytes[10];
};

class Message {
public:
	
	Message() {
	}
	Message(const Message& other) :
		title(other.title),
		data(other.data) {
	}
	Message(Message&& other) :
		title(other.title),
		data(other.data) {
	}
	Message(const std::string& title, const std::vector<uint8_t>& data) :
		title(title), data(data) {
	}
	Message(const std::string& title, void* data, int dataLength) {
		this->title = title;
		this->data.resize(dataLength);
		if(data && dataLength>0) 
			memcpy(&(this->data.front()), data, dataLength);
	}
	Message(const std::string& title) :
		title(title) {
	}
	Message(const std::string& title, const std::string& data) :
		title(title), data(data.begin(), data.end()) {
		this->data.push_back(0);
	}
	
	inline Message& operator = (const Message& other) {
		title = other.title;
		data = other.data;
		return *this;
	}
	
	inline void Swap(Message& other) {
		std::swap(title, other.title);
		std::swap(data, other.data);
	}
	
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

inline void CreateOptimalBuffer(const Message& msg,
		std::vector<uint8_t>& buffer) {
	NumberBuffer size(msg.title.size()+1 + msg.data.size());
	size_t sizebytes = size.GetOccupiedBytes();
	buffer.clear();
	buffer.reserve(size.GetValue() + sizebytes);
	buffer.insert(buffer.end(), size.GetData(), size.GetData()+sizebytes);
	buffer.insert(buffer.end(), msg.title.begin(), msg.title.end());
	buffer.emplace_back(0);
	buffer.insert(buffer.end(), msg.data.begin(), msg.data.end());
}

inline bool CanReadFullMessage(const uint8_t* buffer, uint64_t bufferSize) {
	NumberBuffer size;
	size_t sizebytes = size.SetBytes(buffer, bufferSize);
	if(sizebytes == 0)
		return false;
	if(bufferSize >= size.GetValue()+sizebytes)
		return true;
	return false;
}

inline bool CanReadFullMessage(const std::vector<uint8_t>& buffer) {
	return CanReadFullMessage(&buffer.front(), buffer.size());
}

inline uint64_t TryReadMessageFromBuffer(Message& msg,
		uint8_t* buffer, uint64_t bufferSize) {
	NumberBuffer size;
	size_t sizebytes = size.SetBytes(buffer, bufferSize);
	if(sizebytes == 0)
		return 0;
	if(bufferSize >= size.GetValue()+sizebytes) {
		msg.title = (char*)buffer+sizebytes;
		msg.data.clear();
		msg.data.insert(msg.data.end(),
				buffer + sizebytes + msg.title.size()+1,
				buffer + sizebytes + size.GetValue());
		return sizebytes + size.GetValue();
	}
	return 0;
}

inline uint64_t TryReadMessageFromBuffer(Message& msg,
		std::vector<uint8_t>& buffer) {
	return TryReadMessageFromBuffer(msg, &buffer.front(), buffer.size());
}

#endif

