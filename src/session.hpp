#ifndef SESSION_HPP
#define SESSION_HPP

#include <iostream>
#include <functional>
#include <optional>

#include "net.hpp"
#include "debug.hpp"

class session;

using on_connect_t = std::function<void(std::shared_ptr<session>)>;
using on_disconnect_t = std::function<void(std::shared_ptr<session>)>;
using on_message_t = std::function<void(const std::string &)>;
using on_error_t = std::function<void(const boost::system::error_code &)>;

class session: public std::enable_shared_from_this<session> {
protected:
	session() = default;

	void on_socket_upgrade(const beast::error_code &ec) {
		if (ec) {
			std::cerr << "error occured in 'session::on_accept': " << ec.what() << std::endl;
			return;
		}

		__debug("Successfully accepted websocket");

		on_connect(shared_from_this());

		read();
	}

	void on_read(
			const beast::error_code &ec,
			std::size_t bytes_transferred)
	{
		if (ec == asio::error::eof ||
				ec == asio::error::not_connected ||
				ec == websocket::error::closed ||
				ec == asio::error::operation_aborted) {
			__debug("Session closed");
			on_disconnect(shared_from_this());
			return;
		}

		if (ec) {
			std::cerr
				<< "Error occured in 'session::on_read': "
				<< ec.what()
				<< std::endl;
			on_disconnect(shared_from_this());
			return;
		}

		auto s = beast::buffers_to_string(m_buf.cdata());

		m_buf.consume(m_buf.size());

		on_message(s);

		read();
	}

	void on_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
		if (error) {
			std::cerr << "Error occured in 'session::on_write': " << error.what() << std::endl;
			on_disconnect(shared_from_this());
			return;
		}

		__debug("Written:", bytes_transferred, "bytes");
	}

	void on_close_socket(const beast::error_code &error) {
		if (error && error != beast::websocket::error::closed) {
			std::cerr
				<< "Error occured in 'session::on_close_socket': "
				<< error.what()
				<< std::endl;
		}
	}
protected:
	beast::flat_buffer m_buf;

	virtual void close_socket(beast::websocket::close_reason const &cr) = 0;
	virtual void read() = 0;
	virtual void socket_upgrade() = 0;
public:
	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_message_t on_message;
	on_error_t on_error;

	virtual void write(const std::string &s) = 0;

	void run() {
		socket_upgrade();
	}

	void close() {
		close_socket(beast::websocket::normal);
	}
};


class plain_session
	: public session
{
public:
	plain_session(asio::ip::tcp::socket &&sock)
		: m_ws(std::move(sock))
	{}

	plain_session(asio::ip::tcp::socket &&sock, std::optional<http::request<http::string_body>> req)
		: m_ws(std::move(sock)), m_req(req)
	{}

	void write(const std::string &s) override {
		asio::const_buffer buf = asio::buffer(s);
		m_ws.async_write
			(buf,
			 beast::bind_front_handler(&plain_session::on_write, shared_from_this()));
	}

	static
	std::shared_ptr<plain_session>
	create(asio::ip::tcp::socket &&sock,
		   std::optional<http::request<http::string_body>> req = {})
	{
		return std::make_shared<plain_session>
			(plain_session
			 (std::forward<asio::ip::tcp::socket>(sock), req));
	}
protected:
	void close_socket(beast::websocket::close_reason const &cr) override {
		m_ws.async_close
			(cr,
			 beast::bind_front_handler(&plain_session::on_close_socket, shared_from_this())
			 );
	}

	void read() override {
		m_ws.async_read
			(m_buf,
			 beast::bind_front_handler(&plain_session::on_read, shared_from_this())
			 );
	}

	void socket_upgrade() override {
		if (m_req) {
			m_ws.async_accept
				(m_req.value(), beast::bind_front_handler(&plain_session::on_socket_upgrade, shared_from_this())
				 );
		} else {
			m_ws.async_accept
				(beast::bind_front_handler(&plain_session::on_socket_upgrade, shared_from_this())
				 );
		}
	}

private:
	beast::websocket::stream<beast::tcp_stream> m_ws;

	/*
	  @ if socket was created via authentication middleware, this will contain http::request<http::string_body>
	  @ otherwise it will not contain value and async_accept should be called without it
	*/
	std::optional<http::request<http::string_body>> m_req;
};

#endif
