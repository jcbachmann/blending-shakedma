#include "LineInputHandler.hpp"

LineInputHandler::LineInputHandler()
	: signal_set(io_service)
	, in(io_service, dup(STDIN_FILENO))
	, running(true)
{
	signal_set.add(SIGTERM);
	signal_set.add(SIGINT);
	signal_set.async_wait([this] (boost::system::error_code /*err*/, int /*signal*/) {
		running = false;
	});
	do_read();
}

bool LineInputHandler::operator>>(std::string& line)
{
	while (running && str.empty()) {
		io_service.run_one();
	}
	if (running) {
		std::swap(line, str);
		str.clear();
	}
	return running;
}

void LineInputHandler::do_read()
{
	boost::asio::async_read_until(in, streambuf, '\n', [this] (boost::system::error_code /*err*/, size_t size) {
		if (size) {
			{
				std::istream istream(&streambuf);
				std::getline(istream, str, '\n');
			}
			do_read();
		} else {
			running = false;
		}
	});
}
