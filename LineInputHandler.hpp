#ifndef LINEINPUTHANDLER_HPP
#define LINEINPUTHANDLER_HPP

#include <boost/asio.hpp>

class LineInputHandler
{
	public:
		LineInputHandler();
		bool operator>>(std::string& line);
	private:
		void do_read();
		boost::asio::io_service io_service;
		boost::asio::signal_set signal_set;
		boost::asio::posix::stream_descriptor in;
		boost::asio::streambuf streambuf;
		std::string str;
		bool running;
};

#endif // LINEINPUTHANDLER_HPP
