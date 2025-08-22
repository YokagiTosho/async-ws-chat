#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>

#include "session.hpp"
#include "sessions_manager.hpp"
#include "authentication.hpp"

class server : public std::enable_shared_from_this<server> {
private:
	asio::io_context &m_ioc;
	asio::ip::tcp::acceptor m_acceptor;
	sessions_manager m_sm;
	bool m_shutting_down { false };

	// create only through server::create
	server(asio::io_context &ioc, const asio::ip::tcp::endpoint &ep, int backlog)
		: m_ioc(ioc), m_acceptor(ioc, ep)
	{
		m_acceptor.listen(backlog);
		__debug("Listening", ep);
	}

	void do_accept() {
		m_acceptor.async_accept(
				beast::bind_front_handler(
					&server::on_accept,
					shared_from_this()
					)
				);
	}

	void on_accept(const boost::system::error_code& error, asio::ip::tcp::socket sock) {
		if (error == asio::error::operation_aborted) {
			m_acceptor.close();
			return;
		} else if (error) {
			std::cerr << "Error occured in 'server::on_accept': " << error.what() << std::endl;
			do_accept();
			return;
        }

		/*
		  @ Run authentication middleware.
		  @ Because of move should be called last if more middlewares will present
		 */
		std::make_shared<authentication>
			(authentication
			 (
			  std::move(sock),
			  [this](session &s) {
				  register_callbacks(s);
			  })
			 )->run();

		do_accept();
	}

	void run_session(asio::ip::tcp::socket &&sock) {
		auto s = plain_session::create(std::forward<asio::ip::tcp::socket>(sock));

		register_callbacks(*s);

		s->run();
	}

	void register_callbacks(session &s) {
		s.on_message = [this](const std::string &msg) {
			m_sm.broadcast(msg);
		};

		s.on_connect = [this](std::shared_ptr<session> s) {
			m_sm.add(s);
		};

		s.on_disconnect = [this](std::shared_ptr<session> s) {
			if (!m_shutting_down)
				m_sm.remove(s);
		};

		s.on_error = [this](const boost::system::error_code& error) {
			std::cerr << "Error occured in session: " << error.what() << std::endl;
		};
	}
public:
	static
	std::shared_ptr<server>
	create(asio::io_context &ioc,
		   const asio::ip::tcp::endpoint &ep,
		   int backlog = 30)
	{
		return std::make_shared<server>(server(ioc, ep, backlog));
	}

	void run() {
		do_accept();
	}

	void stop() {
		m_acceptor.cancel();
		m_acceptor.close();

		m_shutting_down = true;

		m_sm.close_sessions();
	}
};

#endif
