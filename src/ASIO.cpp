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

#include "ASIO.hpp"

boost::asio::io_context *ioContext = NULL;
boost::asio::io_context& IoContext() {
	if(ioContext == NULL)
		ioContext = new boost::asio::io_context;
	return *ioContext;
}
void IoContextPollOne() {
	IoContext().poll_one();
}


Endpoint::Endpoint() : port(0) {
}

Endpoint::Endpoint(const char* ip, uint16_t port) :
	ip(ip), port(port) {
}

Endpoint::Endpoint(const char* ipport) {
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

Endpoint::Endpoint(boost::asio::ip::tcp::endpoint tcp) {
	ip = tcp.address().to_string();
	port = tcp.port();
}

Endpoint::Endpoint(boost::asio::ip::udp::endpoint udp) {
	ip = udp.address().to_string();
	port = udp.port();
}

boost::asio::ip::udp::endpoint Endpoint::UdpEndpoint() const {
	if(*this)
		return boost::asio::ip::udp::endpoint(
				boost::asio::ip::address::from_string(ip.c_str()),
				port);
	else
		return boost::asio::ip::udp::endpoint(
				boost::asio::ip::udp::v4(),
				port);
}

boost::asio::ip::tcp::endpoint Endpoint::TcpEndpoint() const {
	if(*this)
		return boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address::from_string(ip.c_str()),
				port);
	else
		return boost::asio::ip::tcp::endpoint(
				boost::asio::ip::tcp::v4(),
				port);
}


bool Endpoint::operator == (const Endpoint& other) const {
	return ip==other.ip && port==other.port;
}

bool Endpoint::operator < (const Endpoint& other) const {
	return ip<other.ip || (ip==other.ip && port<other.port);
}

Endpoint::operator bool() const {
	return ip!="";
}

std::string Endpoint::ToString() const {
	return ip + ":" + std::to_string(port);
}



NumberBuffer::NumberBuffer() {
	value = 0;
	*((uint64_t*)bytes) = 0;
	bytes[8] = 0;
	bytes[9] = 0;
}

NumberBuffer::NumberBuffer(uint64_t val) {
	SetValue(val);
}

NumberBuffer::NumberBuffer(uint8_t* data, size_t max) {
	SetBytes(data, max);
}

NumberBuffer::NumberBuffer(const NumberBuffer& other) {
	*(SIZE__*)this = *(SIZE__*)&other;
}

NumberBuffer::NumberBuffer(NumberBuffer&& other) {
	*(SIZE__*)this = *(SIZE__*)&other;
}

NumberBuffer& NumberBuffer::operator = (const NumberBuffer& other) {
	*(SIZE__*)this = *(SIZE__*)&other;
	return *this;
}

size_t NumberBuffer::GetOccupiedBytes() const {
	int i=0;
	for(; i<10 && bytes[i]&128; ++i) {
	}
	return i+1;
}

const uint8_t* NumberBuffer::GetData() const {
	return bytes;
}

uint64_t NumberBuffer::GetValue() const {
	return value;
}

size_t NumberBuffer::SetValue(uint64_t val) {
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

size_t NumberBuffer::SetBytes(const uint8_t* data, size_t max) {
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

void NumberBuffer::Print() {
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



Message::Message() {
}

Message::Message(const Message& other) :
	title(other.title),
	data(other.data) {
}

Message::Message(Message&& other) :
	title(other.title),
	data(other.data) {
}

Message::Message(const std::string& title, const std::vector<uint8_t>& data) :
	title(title), data(data.begin(), data.end()) {
}

Message::Message(const std::string& title, void* data, int dataLength) {
	this->title = title;
	this->data.resize(dataLength);
	if(data && dataLength>0) 
		memcpy(&(this->data.front()), data, dataLength);
}

Message::Message(const std::string& title) :
	title(title) {
}

Message::Message(const std::string& title, const std::string& data) :
	title(title), data(data.begin(), data.end()) {
	this->data.push_back(0);
}

Message& Message::operator = (const Message& other) {
	title = other.title;
	data = other.data;
	return *this;
}

void Message::Swap(Message& other) {
	std::swap(title, other.title);
	std::swap(data, other.data);
}



void CreateOptimalBuffer(const Message& msg,
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

bool CanReadFullMessage(const uint8_t* buffer, uint64_t bufferSize) {
	NumberBuffer size;
	size_t sizebytes = size.SetBytes(buffer, bufferSize);
	if(sizebytes == 0)
		return false;
	if(bufferSize >= size.GetValue()+sizebytes)
		return true;
	return false;
}

bool CanReadFullMessage(const std::vector<uint8_t>& buffer) {
	return CanReadFullMessage(&buffer.front(), buffer.size());
}

uint64_t TryReadMessageFromBuffer(Message& msg,
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

uint64_t TryReadMessageFromBuffer(Message& msg,
		std::vector<uint8_t>& buffer) {
	return TryReadMessageFromBuffer(msg, &buffer.front(), buffer.size());
}

