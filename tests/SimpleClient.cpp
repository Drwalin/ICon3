
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <array>
#include <string>
#include <iostream>
#include <cstdio>

#include <windows.h>

unsigned GetAvailableBytesToRead(boost::asio::ip::tcp::socket &tcp_socket) {
	boost::asio::socket_base::bytes_readable command(true);
	tcp_socket.io_control(command);
	return command.get();
}

int main() {
	boost::asio::io_service ioservice;
	ioservice.run();
	
	boost::asio::ip::tcp::socket tcp_socket(ioservice);
	std::array<char, 4096> bytes;
	
	boost::system::error_code ec;
	tcp_socket.connect(
			boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address::from_string("127.0.0.1"),
				27000),
			ec);
	
	if(!ec) {
		std::string r = "String od klienta";
		
		printf("\n Bytes to read: %u", GetAvailableBytesToRead(tcp_socket));
		
		Sleep(1000);
		
		printf("\n Bytes to read: %u\n", GetAvailableBytesToRead(tcp_socket));
		
		std::size_t readed = tcp_socket.read_some(boost::asio::buffer(bytes));
		std::cout.write(bytes.data(), readed);
		
		tcp_socket.write_some(boost::asio::buffer(r));
		
		readed = tcp_socket.read_some(boost::asio::buffer(bytes));
		std::cout.write(bytes.data(), readed);
		
		tcp_socket.write_some(boost::asio::buffer(r));
	}
	
	return 0;
}

