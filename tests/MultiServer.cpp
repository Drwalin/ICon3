
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <ctime>
#include <iostream>
#include <cstring>
#include <cstdio>

#include <thread>

void Func(boost::asio::ip::tcp::socket * tcp_socket, boost::system::error_code ec) {
	printf("\n");
	
	std::string data;
	std::array<char, 4096> bytes;
	data.reserve(128);
	
	if(!ec) {
		std::size_t readed;
		data = "String od servera";
		
		tcp_socket->write_some(boost::asio::buffer(data));
		
		readed = tcp_socket->read_some(boost::asio::buffer(bytes));
		std::cout.write(bytes.data(), readed);
		
		tcp_socket->write_some(boost::asio::buffer(data));
		
		readed = tcp_socket->read_some(boost::asio::buffer(bytes));
		std::cout.write(bytes.data(), readed);
		
		tcp_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
		tcp_socket->close();
	}
	
	delete tcp_socket;
}

int main() {
	boost::asio::io_service ioservice;
	ioservice.run();
	
	boost::asio::ip::tcp::endpoint tcp_endpoint(boost::asio::ip::tcp::v4(), 27000);
	boost::asio::ip::tcp::acceptor tcp_acceptor(ioservice, tcp_endpoint);
	
	while(true) {
		boost::asio::ip::tcp::socket * tcp_socket = new boost::asio::ip::tcp::socket(ioservice);
		boost::system::error_code ec;
		tcp_acceptor.accept(*tcp_socket, ec);
		if(!ec) {
			std::thread(Func, tcp_socket, ec).detach();
		} else {
			delete tcp_socket;
		}
	}
	
	return 0;
}

