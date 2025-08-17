#ifndef SESSION_HPP
#define SESSION_HPP

#include <iostream>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

class session;

using on_connect_t = std::function<void(std::shared_ptr<session>)>;
using on_disconnect_t = std::function<void(std::shared_ptr<session>)>;
using on_message_t = std::function<void(const std::string &)>;
using on_error_t = std::function<void(const boost::system::error_code &)>;

// int as a key is temporary solution, TODO change later
using sessions_t = std::unordered_map<int, std::shared_ptr<session>>;

class session: public std::enable_shared_from_this<session> {
private:
	beast::websocket::stream<beast::tcp_stream> m_ws;
	beast::flat_buffer m_buf;

	session(net::ip::tcp::socket &&sock)
		: m_ws(std::move(sock))
	{}

	void socket_upgrade() {
		m_ws.async_accept(
				beast::bind_front_handler(
					&session::on_socket_upgrade,
					shared_from_this()
					)
				);
	}

	void on_socket_upgrade(const beast::error_code &ec) {
		if (ec) {
			std::cerr << "error occured in 'session::on_accept': " << ec.what() << std::endl;
			return;
		}

		std::cout << "Successfully accepted websocket" << std::endl;

		on_connect(shared_from_this());

		do_read();
	}

	void do_read() {
		m_ws.async_read(m_buf,
				beast::bind_front_handler(
					&session::on_read,
					shared_from_this()
					)
				);
	}

	void close_socket(beast::websocket::close_reason const &cr) {
		m_ws.async_close(
				cr,
				beast::bind_front_handler(
					&session::on_close_socket, shared_from_this()
					)
				);
	}

	void on_close_socket(const beast::error_code &error) {
		std::cout << "and here?" << std::endl;
		if (error && error != beast::websocket::error::closed) {
			std::cerr
				<< "Error occured in 'session::on_close_socket': "
				<< error.what()
				<< std::endl;
		}
	}

	void on_read(
			const beast::error_code &ec,
			std::size_t bytes_transferred)
	{
		// TODO move to single if statement, now for debug purposes
		if (ec == boost::asio::error::eof) {
			std::cout << "eof 'session::on_read'" << std::endl;
			on_disconnect(shared_from_this());
			return;
		}
		if (ec == boost::asio::error::not_connected) {
			std::cout << "not_connected 'session::on_read'" << std::endl;
			on_disconnect(shared_from_this());
			return;
		}
		if (ec == websocket::error::closed) {
			std::cout << "Client closed connection" << std::endl;
			on_disconnect(shared_from_this());
			return;
		}

		if (ec == boost::asio::error::operation_aborted) {
			std::cout << "Operation aborted 'session::on_read'" << std::endl;
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

		do_read();
	}

	void on_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
		if (error) {
			std::cerr << "Error occured in 'session::on_write': " << error.what() << std::endl;
			on_disconnect(shared_from_this());
			return;
		}
		std::cout << "Written: " << bytes_transferred << " bytes" << std::endl;
	}
public:
	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_message_t on_message;
	on_error_t on_error;

	static std::shared_ptr<session> create(net::ip::tcp::socket &&sock) {
		return std::make_shared<session>(session(
					std::forward<net::ip::tcp::socket>(sock)));
	}

	void run() {
		socket_upgrade();
	}

	void do_write(const std::string &s) {
		boost::asio::const_buffer buf = boost::asio::buffer(s);

		m_ws.async_write(
				buf,
				beast::bind_front_handler(&session::on_write, shared_from_this()));
	}

	void close() {
		close_socket(boost::beast::websocket::normal);
	}
};

#endif
