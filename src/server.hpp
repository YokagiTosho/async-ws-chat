#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "session.hpp"
#include "sessions_manager.hpp"

namespace beast = boost::beast;
namespace net = boost::asio;

class server : public std::enable_shared_from_this<server> {
private:
	net::io_context &m_ioc;
	net::ip::tcp::acceptor m_acceptor;
	sessions_manager m_sm;
	
	void do_accept() {
		m_acceptor.async_accept(
				beast::bind_front_handler(
					&server::on_accept,
					shared_from_this()
					)
				);
	}

	void on_accept(const boost::system::error_code& error, net::ip::tcp::socket sock) {
		if (error) {
			std::cerr << "err occured in 'server::on_accept': " << error.what() << std::endl;
			do_accept();
			return;
		}

		auto s = session::create(
				std::move(sock));

		s->on_message = [this](const std::string &msg) {
			m_sm.broadcast(msg);
		};

		s->on_connect = [this](std::shared_ptr<session> s) {
			m_sm.add(s);
		};

		s->on_disconnect = [this](auto s) {
			m_sm.remove(s);
		};

		s->on_error = [this](const boost::system::error_code& error) {
			std::cerr << "All occured in session: " << error.what() << std::endl;
		};

		s->run();

		do_accept();
	}

	// create only through server::create
	server(
			net::io_context &ioc,
			net::ip::tcp::acceptor &&acc)
		: m_ioc(ioc), m_acceptor(std::forward<net::ip::tcp::acceptor>(acc))
	{}

public:
	static std::shared_ptr<server> create(
			net::io_context &ioc,
			net::ip::tcp::acceptor &&acc) {
		return std::make_shared<server>(
				server(
					ioc,
					std::forward<net::ip::tcp::acceptor>(acc)
					)
				);
	}

	void run() {
		do_accept();
	}
};

#endif
