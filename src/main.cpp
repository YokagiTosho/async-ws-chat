#include "net.hpp"
#include "server.hpp"
#include "debug.hpp"

int main(int argc, char **argv) {

	asio::io_context ioc{1};

	try {
		asio::ip::tcp::endpoint ep{
			asio::ip::make_address("127.0.0.1"),
				8080
		};


		asio::ip::tcp::acceptor acceptor{ioc, ep};

		constexpr int backlog = 30;

		acceptor.listen(backlog);

		__debug("Listening", ep);

		auto s = server::create(ioc, std::move(acceptor));
		s->run();

		boost::asio::signal_set signals{ioc, SIGINT};
		signals.async_wait([&s](
					const boost::system::error_code& error,
					int signal_number) {
				if (error) {
					std::cerr
					<< "Error occured in signal handler: "
					<< error.what()
					<< std::endl;
				}

				if (signal_number == SIGINT || signal_number == SIGTERM) {
					s->stop();
				}

				});

		ioc.run();
	} catch (const boost::system::system_error &e) {
		std::cerr << "Exception occured in 'main': " << e.what() << ": " << e.code() << std::endl;
		exit(1);
	} catch (const std::exception &e) {
		ioc.stop();
	}

	return 0;
}
