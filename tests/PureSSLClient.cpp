//
// client.cpp
// ~~~~~~~~~~
//
// Copyright(c) 2003-2020 Christopher M. Kohlhoff(chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0.(See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

enum { max_length = 1024 };

class client {
public:
	client(boost::asio::io_context& io_context, boost::asio::ssl::context& context, const tcp::resolver::results_type& endpoints) :
			socket_(io_context, context) {
		socket_.set_verify_mode(boost::asio::ssl::verify_peer);
		//socket_.set_verify_callback(std::bind(&client::verify_certificate, this, _1, _2));
		connect(endpoints);
	}
	
private:
	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx) {
		// The verify callback can be used to check whether the certificate that is
		// being presented is valid for the peer. For example, RFC 2818 describes
		// the steps involved in doing this for HTTPS. Consult the OpenSSL
		// documentation for more details. Note that the callback is called once
		// for each certificate in the certificate chain, starting from the root
		// certificate authority.
		
		// In this example we will simply print the certificate's subject name.
		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		std::cout << "Verifying " << subject_name << "\n";
					printf("\n preverified: %s", preverified?"true":"false");
		
		return preverified;
	}
	
	void connect(const tcp:: resolver::results_type& endpoints) {
		boost::system::error_code err;
		socket_.lowest_layer().connect(*endpoints, err);
		if(err) {
			std::cout << "Connect failed: " << err.message() << "\n";
			return;
		}
		
		socket_.handshake(boost::asio::ssl::stream_base::client, err);
		if(err) {
			std::cout << "Handshake failed: " << err.message() << "\n";
			return;
		}
		
		std::cout << "Enter message: ";
		std::cin.getline(request_, max_length);
		size_t request_length = std::strlen(request_);
		
		socket_.async_read_some(boost::asio::buffer(reply_, request_length),
				[this](const boost::system::error_code& ec, size_t length) {
					if(!ec) {
						std::cout << "Reply: ";
						std::cout.write(reply_, length);
						std::cout << "\n";
					}
				});
		
		size_t length = socket_.write_some(boost::asio::buffer(request_, request_length), err);
		if(err) {
			std::cout << "Write failed: " << err.message() << "\n";
			return;
		}
	}
	
	boost::asio::ssl::stream<tcp::socket> socket_;
	char request_[max_length];
	char reply_[max_length];
};

int main(int argc, char* argv[]) {
	try {
		if(argc != 3) {
			std::cerr << "Usage: client <host> <port>\n";
			return 1;
		}
		
		boost::asio::io_context io_context;
		
		tcp::resolver resolver(io_context);
		auto endpoints = resolver.resolve(argv[1], argv[2]);
		
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		ctx.load_verify_file("cert/rootca.crt");
		
		client c(io_context, ctx, endpoints);
		io_context.run();
		io_context.run();
	} catch(std::exception& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	}
	
	return 0;
}
