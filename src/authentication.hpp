#ifndef AUTHENTICATION_HPP
#define AUTHENTICATION_HPP

#include <functional>
#include <stdexcept>
#include <system_error>

#include "jwt-cpp/jwt.h"

#include "net.hpp"
#include "session.hpp"
#include "debug.hpp"

class jwt_verifier {
public:
	class error {
	public:
		enum class code {
			ok,
			token_incorrect_format_error,
			token_verification_error,
			base64_decode_error,
		};

		error(code ec = code::ok)
			: m_ec(ec)
		{}

		error(const error &) = default;

		error(error &&) = default;

		error &operator=(code ec) {
			m_ec = ec;

			return *this;
		}

		explicit operator bool() const
		{ return m_ec != code::ok; }

		const std::string what() const {
		    switch (m_ec) {
			case code::ok: return "ok";
			case code::token_incorrect_format_error: return "Token incorrect format";
			case code::token_verification_error: return "Token verification error";
			case code::base64_decode_error: return "Base64 decode error";
			}

			return "Undefined error code";
		}

		code value() {
			return m_ec;
		}
	private:
		code m_ec;
	};

	explicit jwt_verifier(const std::string &token)
		: m_token(token)
	{}

	bool verify(jwt_verifier::error &ec) {
		ec = error::code::ok;

		try {
			auto decoded = jwt::decode(m_token);

			// for (auto& e : decoded.get_payload_json())
			// 	std::cout << e.first << " = " << e.second << '\n';

			auto verifier = jwt::verify()
				.allow_algorithm(jwt::algorithm::hs256{"MySecretSecret123"});

			std::error_code _ec;
			verifier.verify(decoded, _ec);

			if (_ec) {
				std::cerr << "Failed to verify token: " << _ec.message() << std::endl;
				ec = error::code::token_verification_error;
				return false;
			}

		} catch (const std::invalid_argument &exc) {
			ec = error::code::token_incorrect_format_error;
			return false;
		} catch (const std::runtime_error &exc) {
			ec = error::code::base64_decode_error;
			return false;
		}

		return true;
	}
private:
	const std::string &m_token;
};

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

		jwt_verifier verifier(token);

		jwt_verifier::error ec;
		verifier.verify(ec);

		if (ec) {
			std::cerr << "Could not verify token: " << ec.what() << std::endl;
		}

		__debug("Successfully verified token");

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
