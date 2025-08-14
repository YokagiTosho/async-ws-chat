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

using msg_notify_callback = std::function<void(const std::string &)>;

using on_connect_t = std::function<void(std::shared_ptr<session>)>;
using on_disconnect_t = std::function<void(size_t)>;
using on_message_t = std::function<void(const std::string &)>;
using on_error_t = std::function<void(const boost::system::error_code &)>;

class session: public std::enable_shared_from_this<session> {
private:
	beast::websocket::stream<beast::tcp_stream> m_ws;
	beast::flat_buffer m_buf;
	size_t m_id { 0 };

	session(size_t id, net::ip::tcp::socket &&sock)
		: m_id(id), m_ws(std::move(sock))
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

	void on_read(
			const beast::error_code &ec,
			std::size_t bytes_transferred)
	{
		if(ec == websocket::error::closed) {
			// client closed session
			std::cout << "Client closed connection" << std::endl;
			on_disconnect(m_id);
			return;
		}

		if (ec) {
			// some other error occured
			std::cerr << "Error occured in 'session::on_read': " << ec.what() << std::endl;
			do_read();
			return;
		}

		auto s = beast::buffers_to_string(m_buf.cdata());

		on_message(s);

		m_buf.consume(m_buf.size());

		do_read();
	}

	void on_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
		if (error) {
			std::cerr << "Error occured in 'session::on_write': " << error.what() << std::endl;
			return;
		}
		std::cout << "Successfully written " << bytes_transferred << " bytes" << std::endl;
	}
public:
	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_message_t on_message;
	on_error_t on_error;

	static std::shared_ptr<session> create(size_t id, net::ip::tcp::socket &&sock) {
		return std::make_shared<session>(session(
					id,
					std::forward<net::ip::tcp::socket>(sock)));
	}

	void run() {
		socket_upgrade();
	}

	void do_write(const std::string &s) {
		std::cout << "send msg: " << s << std::endl;
		
		auto buf = boost::asio::buffer(s);
		
		m_ws.async_write(
				buf,
				beast::bind_front_handler(&session::on_write, this));
	}

	const size_t id() const {
		return m_id;
	}

};

// int as a key is temporary solution, TODO change later
using sessions_t = std::unordered_map<int, std::shared_ptr<session>>;

#endif
