//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <windows.h>

using boost::asio::ip::udp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

int main()
{
	std::cout << "\nStart!\n\n";
  try
  {
    boost::asio::io_service io_service;

    udp::socket socket(io_service, udp::endpoint(udp::v4(), 13456));

    for (;;)
    {
      boost::array<char, 128> recv_buf;
      udp::endpoint remote_endpoint;
      boost::system::error_code error;
	  size_t len;
	  
	  Sleep(1000);
	  Sleep(1000);
	  
	  std::cout << "\n Available: " << socket.available() << "\n";
	  
      len = socket.receive_from(boost::asio::buffer(recv_buf),
          remote_endpoint, 0, error);
    std::cout.write(recv_buf.data(), len);
	std::cout << "\n";
	  
	  std::cout << "\n Available: " << socket.available() << "\n";
	  Sleep(1000);
	  std::cout << "\n Available: " << socket.available() << "\n";
      len = socket.receive_from(boost::asio::buffer(recv_buf),
          remote_endpoint, 0, error);
    std::cout.write(recv_buf.data(), len);
	std::cout << "\n";
	  
	  std::cout << "\n Available: " << socket.available() << "\n";
      len = socket.receive_from(boost::asio::buffer(recv_buf),
          remote_endpoint, 0, error);
    std::cout.write(recv_buf.data(), len);
	std::cout << "\n";
	std::cout << "\n";
	  std::cout << "\n Available: " << socket.available() << "\n";

      if (error && error != boost::asio::error::message_size)
        throw boost::system::system_error(error);

      std::string message = make_daytime_string();

      boost::system::error_code ignored_error;
      socket.send_to(boost::asio::buffer(message),
          remote_endpoint, 0, ignored_error);
		  
		  Sleep(1100);
		  
      message = make_daytime_string();
      socket.send_to(boost::asio::buffer(message),
          remote_endpoint, 0, ignored_error);
		  
		  Sleep(1100);
		  
      message = make_daytime_string();
      socket.send_to(boost::asio::buffer(message),
          remote_endpoint, 0, ignored_error);
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}