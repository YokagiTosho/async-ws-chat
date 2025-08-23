#ifndef AUTHENTICATION_HPP
#define AUTHENTICATION_HPP

#include "net.hpp"
#include "session.hpp"
#include "debug.hpp"

#include "jwt-cpp/jwt.h"
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <functional>

using on_authenticate = std::function<void(session &)>;

class authentication : public std::enable_shared_from_this<authentication> {
public:
	authentication(asio::ip::tcp::socket&& sock, on_authenticate on_auth)
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
			__debug("Not an upgrade request");
			return;
		}

		auto authorization_header = m_req.find(http::field::authorization);

		if (authorization_header == m_req.end()) {
			__debug("Authorization token is not present");
			return;
		}

		bool res;
		std::string token = extract_token((*authorization_header).value(), res);

		if (!res) {
			return;
		}

		// TODO validate JWT token here
		if (!validate_token(token)) {
			// not a valid token
			return;
		}

		run_session();
	}

	std::string extract_token(const std::string_view &v, bool &out) {
		auto it = std::find(v.begin(), v.end(), ' ');
		if (it == v.end()) {
			__debug("Invalid Authorization header");
			out = false;
			return {};
		}

		std::string token(it+1, v.end());

		token.erase(std::remove(token.begin(), token.end(), ' '), token.end());

		out = true;

		return token;
	}

	bool validate_token(const std::string &token) {
		__debug("Token", token);


		auto decoded = jwt::decode(token);

		for (auto& e : decoded.get_payload_json())
			std::cout << e.first << " = " << e.second << '\n';
		return true;
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
