#define BOOST_TEST_MODULE HTTP_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/http/server.hpp>
#include <zeep/exception.hpp>
#include <zeep/crypto.hpp>

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