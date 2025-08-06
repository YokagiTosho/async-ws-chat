#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "session.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class listener : public std::enable_shared_from_this<listener> {
private:
	net::io_context &m_ioc;
	net::ip::tcp::acceptor m_acceptor;

	void do_accept() {
		m_acceptor.async_accept(beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

	void on_accept(const boost::system::error_code& error, net::ip::tcp::socket sock) {
		std::cout << "on_accept" << std::endl;

		if (error) {
			std::cerr << "err occured in 'listener::on_accept': " << error.what() << std::endl;
			return;
		}

		session::create(std::move(sock))->run();

		do_accept();
	}

	// create only through listener::create
	listener(net::io_context &ioc, net::ip::tcp::acceptor &&acc)
		: m_ioc(ioc), m_acceptor(std::forward<net::ip::tcp::acceptor>(acc))
	{}
public:
	static std::shared_ptr<listener> create(net::io_context &ioc, net::ip::tcp::acceptor &&acc) {
		return std::make_shared<listener>(listener(ioc, std::forward<net::ip::tcp::acceptor>(acc)));
	}

	void run() {
		do_accept();
	}
};


#endif
