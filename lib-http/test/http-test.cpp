#define BOOST_TEST_MODULE HTTP_Test
#include <boost/test/included/unit_test.hpp>

#include <random>

#include <zeep/http/server.hpp>
#include <zeep/exception.hpp>
#include <zeep/crypto.hpp>
#include <zeep/http/daemon.hpp>
#include <zeep/http/message-parser.hpp>
#include <zeep/streambuf.hpp>
#include <zeep/html/controller.hpp>
#include <zeep/html/el-processing.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

namespace z = zeep;
namespace zx = zeep::xml;
namespace zh = zeep::http;
namespace io = boost::iostreams;

BOOST_AUTO_TEST_CASE(http_base64_1)
{
	using namespace std::literals;

	auto in = R"(Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.)"s;
	
	auto out = R"(TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz
IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg
dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu
dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo
ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=
)"s;

	auto test = zeep::encode_base64(in, 76);

	BOOST_TEST(test == out);

	auto s = zeep::decode_base64(test);

	BOOST_TEST(s == in);
}

BOOST_AUTO_TEST_CASE(http_base64_2)
{
	using namespace std::literals;

	const std::string tests[] =
	{
		"1", "12", "123", "1234",
		{ '\0' }, { '\0', '\001' }, { '\0', '\001', '\002' }
	};

	for (const auto& test: tests)
	{
		auto enc = zeep::encode_base64(test, 76);

		auto dec = zeep::decode_base64(enc);

		BOOST_TEST(dec == test);
	}
}

BOOST_AUTO_TEST_CASE(webapp_6)
{
	zh::request req;
	req.set_header("Content-Type", "multipart/form-data; boundary=xYzZY");
	req.payload = "--xYzZY\r\nContent-Disposition: form-data; name=\"pdb-file\"; filename=\"1cbs.cif.gz\"\r\nContent-Encoding: gzip\r\nContent-Type: chemical/x-cif\r\n\r\nhello, world!\n\r\n--xYzZY\r\nContent-Disposition: form-data; name=\"mtz-file\"; filename=\"1cbs_map.mtz\"\r\nContent-Type: text/plain\r\n\r\nAnd again, hello!\n\r\n--xYzZY--\r\n";

	auto fp1 = req.get_file_parameter("pdb-file");
	BOOST_CHECK_EQUAL(fp1.filename, "1cbs.cif.gz");
	BOOST_CHECK_EQUAL(fp1.mimetype, "chemical/x-cif");
	BOOST_CHECK_EQUAL(std::string(fp1.data, fp1.data + fp1.length), "hello, world!\n");

	auto fp2 = req.get_file_parameter("mtz-file");
	BOOST_CHECK_EQUAL(fp2.filename, "1cbs_map.mtz");
	BOOST_CHECK_EQUAL(fp2.mimetype, "text/plain");
	BOOST_CHECK_EQUAL(std::string(fp2.data, fp2.data + fp2.length), "And again, hello!\n");
}

BOOST_AUTO_TEST_CASE(webapp_6a)
{
	char s[] = "Hello, world!";

	io::stream<io::array_source> is(s, s + strlen(s));

	auto len = is.seekg(0, std::ios_base::end).tellg();
	BOOST_CHECK_EQUAL(len, strlen(s));
}

zh::reply simple_request(uint16_t port, const std::string_view& req)
{
	using boost::asio::ip::tcp;

	boost::asio::io_context io_context;

	tcp::resolver resolver(io_context);
	tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", std::to_string(port));

	tcp::socket socket(io_context);
	boost::asio::connect(socket, endpoints);

	boost::system::error_code ignored_error;
	boost::asio::write(socket, boost::asio::buffer(req), ignored_error);

	zh::reply result;
	zh::reply_parser p;

	for (;;)
	{
		boost::array<char, 128> buf;
		boost::system::error_code error;

		size_t len = socket.read_some(boost::asio::buffer(buf), error);

		if (error == boost::asio::error::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		zeep::char_streambuf sb(buf.data(), len);

		auto r = p.parse(result, sb);
		if (r == true)
			break;
	}

	return result;
}

BOOST_AUTO_TEST_CASE(webapp_7)
{
	// start up a http server and stop it again

	zh::daemon d([]() { return new zh::server; }, "zeep-http-test");

	std::random_device rng;
	uint16_t port = 1024 + (rng() % 10240);

	std::thread t(std::bind(&zh::daemon::run_foreground, d, "127.0.0.1", port));

	std::cerr << "started daemon at port " << port << std::endl;

	sleep(1);

	auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");
	BOOST_TEST(reply.get_status() == zh::not_found);

	reply = simple_request(port, "XXX / HTTP/1.0\r\n\r\n");
	BOOST_TEST(reply.get_status() == zh::bad_request);

	pthread_kill(t.native_handle(), SIGHUP);

	t.join();

}


// BOOST_AUTO_TEST_CASE(webapp_8)
// {
// 	// start up a http server with a html_controller and stop it again

// 	class my_html_controller : public zeep::html::controller
// 	{
// 	  public:
// 		my_html_controller()
// 			: zeep::html::controller("/", "")
// 		{
// 			mount("", &my_html_controller::handle_index);
// 		}

// 		void handle_index(const zh::request& req, const zeep::html::scope& scope, zh::reply& rep)
// 		{
// 			rep = zh::reply::stock_reply(zh::ok);
// 			rep.set_content("Hello", "text/plain");
// 		}
// 	};

// 	zh::daemon d([]() {
// 		auto server = new zh::server;
// 		server->add_controller(new my_html_controller());
// 		return server;
// 	}, "zeep-http-test");

// 	std::random_device rng;
// 	uint16_t port = 1024 + (rng() % 10240);

// 	std::thread t(std::bind(&zh::daemon::run_foreground, d, "127.0.0.1", port));

// 	std::cerr << "started daemon at port " << port << std::endl;

// 	sleep(1);

// 	auto reply = simple_request(port, "GET / HTTP/1.0\r\n\r\n");

// 	pthread_kill(t.native_handle(), SIGHUP);

// 	t.join();

// 	BOOST_TEST(reply.get_status() == zh::ok);
// 	BOOST_TEST(reply.get_content() == "Hello");
// }



// {
// 	zh::request req;
// 	req.set_header("Content-Type", "multipart/form-data; boundary=xYzZY");
// 	req.payload = "--xYzZY\r\nContent-Disposition: form-data; name=\"pdb-file\"; filename=\"1cbs.cif.gz\"\r\nContent-Encoding: gzip\r\nContent-Type: chemical/x-cif\r\n\r\nhello, world!\n\r\n--xYzZY\r\nContent-Disposition: form-data; name=\"mtz-file\"; filename=\"1cbs_map.mtz\"\r\nContent-Type: text/plain\r\n\r\nAnd again, hello!\n\r\n--xYzZY--\r\n";

// 	auto fp1 = req.get_file_parameter("pdb-file");
// 	BOOST_CHECK_EQUAL(fp1.filename, "1cbs.cif.gz");
// 	BOOST_CHECK_EQUAL(fp1.mimetype, "chemical/x-cif");

// 	const std::string ks[] = {
// 		"hello, world!\n",
// 		"And again, hello!\n"
// 	};

// 	auto is = fp1.content();
// 	size_t n = ks[0].length();

// 	is.seekg(0, std::ios_base::end);
// 	BOOST_CHECK_EQUAL(is.tellg(), n);

// 	is.seekg(0);
// 	std::string b(n, ' ');
// 	is.read(b.data(), n);
// 	BOOST_CHECK_EQUAL(b, ks[0]);

// 	auto fp2 = req.get_file_parameter("mtz-file");
// 	// BOOST_CHECK_EQUAL(fp1.filename(), "1cbs.cif.gz");
// 	// BOOST_CHECK_EQUAL(fp1.mimetype(), "chemical/x-cif");

// 	auto is2 = fp2.content();
// 	n = ks[1].length();

// 	// is2.seekg(0, std::ios_base::end);
// 	// BOOST_CHECK_EQUAL(is2.tellg(), n);

// 	// is2.seekg(0);
// 	std::string b2(n, ' ');
// 	is2.read(b2.data(), n);
// 	BOOST_CHECK_EQUAL(b2, ks[1]);

// }