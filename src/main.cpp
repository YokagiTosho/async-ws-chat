#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;


class listener : public std::enable_shared_from_this<listener> {
private:
	net::io_context &m_ioc;
	net::ip::tcp::acceptor m_acceptor;

	void do_accept() {
		auto sock = net::ip::tcp::socket(m_ioc);

		m_acceptor.async_accept(beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

	void on_accept(const boost::system::error_code& error, net::ip::tcp::socket sock) {
	//void on_accept(const boost::system::error_code& error) {
		std::cout << "on_accept" << std::endl;

		if (error) {
			std::cerr << "err occured in 'on_accept': " << error.what() << std::endl;
		}


		do_accept();
	}

public:
	listener(net::io_context &ioc, net::ip::tcp::acceptor &&acc)
		: m_ioc(ioc), m_acceptor(std::forward<net::ip::tcp::acceptor>(acc))
	{

	}

	void run() {
		do_accept();
	}
};

int main() {
	std::cout << "main()" << std::endl;

	try {

	net::ip::tcp::endpoint ep{net::ip::make_address("127.0.0.1"), 8080};

	net::io_context ioc{1};

	net::ip::tcp::acceptor acceptor{ioc, ep};

	constexpr int backlog = 30;

	acceptor.listen(backlog);

	std::make_shared<listener>(listener(ioc, std::move(acceptor)))->run();

	ioc.run();

	} catch (const boost::system::system_error &e) {
		std::cerr << "Exception occured: " << e.what() << ": " << e.code() << std::endl;
		exit(1);
	}

	return 0;
}
