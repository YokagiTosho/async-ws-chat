#ifndef AUTHENTICATION_HPP
#define AUTHENTICATION_HPP

#include "net.hpp"
#include "session.hpp"

using on_authenticate = std::function<void(session&)>;

class authentication : public std::enable_shared_from_this<authentication> {
public:
	explicit authentication(asio::ip::tcp::socket&& sock, on_authenticate on_auth)
		: m_sock(std::forward<asio::ip::tcp::socket>(sock)), m_on_auth(on_auth)
	{}

	void run() {
		read_request();
	}

private:
	void read_request() {
		http::async_read
			(m_sock,
			 m_buffer,
			 m_req,
			 beast::bind_front_handler
			 (&authentication::on_read_request, shared_from_this())
			 );
	}

	void on_read_request(const boost::system::error_code &error, std::size_t bytes_transferred) {
		if (!beast::websocket::is_upgrade(m_req)) {
			__debug("Not upgrade request");
			m_sock.cancel();
			m_sock.close();
			return;
		}

		auto r = m_req.find(http::field::authorization);

		if (r == m_req.end()) {
			__debug("Authorization token is not present");
			m_sock.cancel();
			m_sock.close();
			return;
		}

		// TODO validate JWT token here

		run_session();
	}

	void run_session() {
		auto s = plain_session::create(std::move(m_sock), m_req);

		// registers needed callbacks in server class
		m_on_auth(*s);

		s->run();
	}

	asio::ip::tcp::socket m_sock;
	http::request<http::string_body> m_req;
	boost::beast::flat_buffer m_buffer;
	on_authenticate m_on_auth;
};

#endif
