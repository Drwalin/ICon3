//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main(int argc, char* argv[])
{
  try
  {/*
    if (argc != 2)
    {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }
*/
    boost::asio::io_service io_service;
/*
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), argv[1], "daytime");*/
    udp::endpoint receiver_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 13456);//*resolver.resolve(query);

    udp::socket socket(io_service, udp::endpoint(udp::v4(), 0));//, receiver_endpoint);//udp::endpoint(udp::v4(), 12345));
    //socket.open(udp::v4());

	  std::cout << "\n Available: " << socket.available() << "\n";
    std::string send_buf  = "12312h st56je4124";
    socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
    send_buf  = "4523646234236v54y4f35";
    socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
    send_buf  = "12316 3536h jk,u8l de4 8op-0;[;[/ edex24124";
    socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
	  std::cout << "\n Available: " << socket.available() << "\n";

    boost::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    size_t len = socket.receive_from(
        boost::asio::buffer(recv_buf), sender_endpoint);
    std::cout.write(recv_buf.data(), len);
	
	  std::cout << "\n Available: " << socket.available() << "\n";
    len = socket.receive_from(
        boost::asio::buffer(recv_buf), sender_endpoint);
    std::cout.write(recv_buf.data(), len);
	
	  std::cout << "\n Available: " << socket.available() << "\n";
    len = socket.receive_from(
        boost::asio::buffer(recv_buf), sender_endpoint);
    std::cout.write(recv_buf.data(), len);
	  std::cout << "\n Available: " << socket.available() << "\n";
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}