#include "listener.hpp"

int main(int argc, char **argv) {
	try {
		net::ip::tcp::endpoint ep{net::ip::make_address("127.0.0.1"), 8080};

		net::io_context ioc{1};

		net::ip::tcp::acceptor acceptor{ioc, ep};

		constexpr int backlog = 30;

		acceptor.listen(backlog);

		listener::create(ioc, std::move(acceptor))->run();

		ioc.run();
	} catch (const boost::system::system_error &e) {
		std::cerr << "Exception occured: " << e.what() << ": " << e.code() << std::endl;
		exit(1);
	}

	return 0;
}
