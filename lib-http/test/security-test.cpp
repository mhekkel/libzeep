#include <boost/asio.hpp>

#define BOOST_TEST_MODULE Security_Test
#include <boost/test/included/unit_test.hpp>

#include <random>
#include <sstream>

#include <zeep/crypto.hpp>
#include <zeep/exception.hpp>
#include <zeep/streambuf.hpp>

#include <zeep/http/controller.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/login-controller.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/http/security.hpp>
#include <zeep/http/server.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include "client-test-code.hpp"
#include "../src/signals.hpp"

namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;
namespace io = boost::iostreams;

BOOST_AUTO_TEST_CASE(sec_1)
{
	zh::reply rep;

	BOOST_CHECK_THROW(rep = zh::reply::redirect("http://example.com\r\nSet-Cookie: wrong=false;"), zeep::exception);

	rep = zh::reply::redirect("http://example.com");

/*
	std::cerr << rep << std::endl;

	std::ostringstream os;
	os << rep;

	zh::reply_parser p;

	std::string s = os.str();
	zeep::char_streambuf sb(s.c_str(), s.length());

	p.parse(sb);
	auto r2 = p.get_reply();

	std::cerr << r2 << std::endl;

	BOOST_CHECK(r2.get_cookie("wrong").empty());
*/
}

