#ifndef SESSION_HPP
#define SESSION_HPP

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class session: public std::enable_shared_from_this<session> {
private:
	beast::websocket::stream<beast::tcp_stream> m_ws;

	session(net::ip::tcp::socket &&sock)
		: m_ws(std::move(sock))
	{}

	void do_accept() {
		m_ws.async_accept(
				beast::bind_front_handler(
					&session::on_accept,
					shared_from_this()
					)
				);
	}

	void on_accept(const beast::error_code &ec) {
		if (ec) {
			std::cerr << "error occured in 'session::on_accept': " << ec.what() << std::endl;
			return;
		}

		std::cout << "Successfully accepted websocket" << std::endl;

		do_read();
	}


	void do_read() {
		//m_ws.async_read();
	}


	void on_read(
			const beast::error_code &ec,
			std::size_t bytes_transferred) {
		if(ec == websocket::error::closed)
			// client closed session
			return;


	}

	void do_write() {
	}

	void on_write() {
	}
public:
	static std::shared_ptr<session> create(net::ip::tcp::socket &&sock) {
		return std::make_shared<session>(session(std::forward<net::ip::tcp::socket>(sock)));
	}

	void run() {
		do_accept();
	}
};


#endif
