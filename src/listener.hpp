#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "session.hpp"

namespace beast = boost::beast;
namespace net = boost::asio;

class listener : public std::enable_shared_from_this<listener> {
private:
	net::io_context &m_ioc;
	net::ip::tcp::acceptor m_acceptor;
	size_t m_id {0};
	sessions_t m_sessions;
	
	void do_accept() {
		m_acceptor.async_accept(
				beast::bind_front_handler(
					&listener::on_accept,
					shared_from_this()
					)
				);
	}

	void on_accept(const boost::system::error_code& error, net::ip::tcp::socket sock) {
		if (error) {
			std::cerr << "err occured in 'listener::on_accept': " << error.what() << std::endl;

			do_accept();

			return;
		}

		auto s= session::create(
				m_id,
				std::move(sock));

		s->on_message = [this](const std::string &msg) {
			for (auto &[key, session] : m_sessions) {
				session->do_write(msg);
			}
		};

		s->on_connect = [this](std::shared_ptr<session> s) {
			m_sessions[s->id()] = s;
		};

		s->on_error = [this](const boost::system::error_code& error) {

		};

		s->on_disconnect = [this](size_t id) {
			m_sessions.erase(id);
		};

		
		m_id++;

		s->run();

		do_accept();
	}

	void on_notify_message(const std::string &msg) {
		std::cout << "recv msg: " << msg << std::endl;
	}

	// create only through listener::create
	listener(
			net::io_context &ioc,
			net::ip::tcp::acceptor &&acc)
		:
			m_ioc(ioc),
			m_acceptor(std::forward<net::ip::tcp::acceptor>(acc))
	{}
public:
	static std::shared_ptr<listener> create(
			net::io_context &ioc,
			net::ip::tcp::acceptor &&acc) {
		return std::make_shared<listener>(
				listener(
					ioc,
					std::forward<net::ip::tcp::acceptor>(acc)));
	}

	void run() {
		do_accept();
	}
};


#endif
