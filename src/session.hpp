#ifndef SESSION_HPP
#define SESSION_HPP

#include <iostream>
#include <functional>

#include "net.hpp"
#include "debug.hpp"
#include "proto_parser.hpp"

class message_formatter {
public:
	message_formatter()
	{}

	message_formatter &with_timestamp() {

		return *this;
	}

	message_formatter &with_name(const std::string &name) {

		return *this;
	}
};

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

		read();
	}

	void on_read(
				 const beast::error_code &ec,
				 std::size_t bytes_transferred)
	{
		if (ec && is_disconnected(ec)) {
			on_disconnect(shared_from_this());
			return;
		}

		if (ec && is_unhandled_error(ec)) {
			std::cerr << "Error occured in 'session::on_read': " << ec.what() << std::endl;
			on_disconnect(shared_from_this());
			return;
		}

		std::string s = beast::buffers_to_string(m_buf.cdata());
		m_buf.consume(m_buf.size());


		if (m_parser.get_state() != proto_parser::state::ready) {
			auto state = m_parser.next_state(s);

			if (state == proto_parser::state::ready) {
				// parser is ready
				std::cout << "Nick: " << m_parser.nick() << std::endl;
				std::cout << "Room: " << m_parser.room() << std::endl;

				on_connect(shared_from_this());
			}
			read();
			return;
		}

		on_message(nick()+";"+s);
		read();
	}

	bool is_disconnected(const beast::error_code &ec) {
		std::cout << ec.what() << " " << ec.value() << std::endl;
		switch (ec.value()) {
			/* disconnected */
		case static_cast<int>(beast::websocket::error::closed):
		case asio::error::not_connected:
		case asio::error::eof:
		case asio::error::operation_aborted:
			return true;
		}
		return false;
	}

	bool is_unhandled_error(const beast::error_code &ec) {
		switch (ec.value()) {
		case boost::system::errc::success:
		case static_cast<int>(beast::websocket::error::buffer_overflow):
			return false;
		default:
			return true;
		}
	}

	void on_write(const boost::system::error_code& ec, std::size_t bytes_transferred) {
		if (ec && is_disconnected(ec)) {
			on_disconnect(shared_from_this());
			return;
		}

		if (ec && is_unhandled_error(ec)) {
			std::cerr << "Error occured in 'session::on_write': " << ec.what() << std::endl;
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
	const size_t m_buf_size { 20 };
	beast::flat_buffer m_buf { m_buf_size };

	virtual void close_socket(beast::websocket::close_reason const &cr) = 0;
	virtual void read() = 0;
	virtual void socket_upgrade() = 0;
public:
	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_message_t on_message;
	on_error_t on_error;

	virtual void write(const std::string &s) = 0;

	void run() { socket_upgrade(); }

	void close() { close_socket(beast::websocket::normal); }

	const std::string &nick() const { return m_parser.nick(); }

	const std::string &room() const { return m_parser.room(); }
private:
	proto_parser m_parser;
};


class plain_session
	: public session
{
public:
	plain_session(asio::ip::tcp::socket &&sock)
		: m_ws(std::move(sock))
	{}

	void write(const std::string &s) override {
		asio::const_buffer buf = asio::buffer(s);
		m_ws.async_write
			(buf,
			 beast::bind_front_handler(&plain_session::on_write, shared_from_this()));
	}

	static
	std::shared_ptr<plain_session>
	create(asio::ip::tcp::socket &&sock)
	{

		return std::make_shared<plain_session>
			(plain_session
			 (std::forward<asio::ip::tcp::socket>(sock)));
	}
protected:
	void close_socket(beast::websocket::close_reason const &cr) override {
		m_ws.async_close
			(cr,
			 beast::bind_front_handler(&plain_session::on_close_socket, shared_from_this())
			 );
	}

	// This method is called when websocket handshake succeeds
	void read() override {
		m_ws.async_read_some
			(m_buf,
			 m_buf_size,
			 beast::bind_front_handler(&plain_session::on_read, shared_from_this())
			 );
	}

	void socket_upgrade() override {
		m_ws.async_accept
			(beast::bind_front_handler(&plain_session::on_socket_upgrade, shared_from_this())
			 );
	}
private:
	beast::websocket::stream<beast::tcp_stream> m_ws;

};

#endif
