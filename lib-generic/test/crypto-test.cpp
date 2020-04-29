#define BOOST_TEST_MODULE Crypto_Test
#include <boost/test/included/unit_test.hpp>

#include <zeep/exception.hpp>
#include <zeep/crypto.hpp>
#include <zeep/streambuf.hpp>

namespace z = zeep;

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

	BOOST_CHECK_EQUAL(test, out);

	auto s = zeep::decode_base64(test);

	BOOST_CHECK(s == in);
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

		BOOST_CHECK(dec == test);
	}
}

BOOST_AUTO_TEST_CASE(crypto_md5_1)
{
	auto h = z::encode_hex(z::md5("1234"));
	BOOST_CHECK_EQUAL(h, "81dc9bdb52d04dc20036dbd8313ed055");
}

BOOST_AUTO_TEST_CASE(crypto_sha1_1)
{
	auto h = z::encode_hex(z::sha1("The quick brown fox jumps over the lazy dog"));
	BOOST_CHECK_EQUAL(h, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

BOOST_AUTO_TEST_CASE(crypto_sha256_1)
{
	auto h = z::encode_hex(z::sha256(""));
	BOOST_CHECK_EQUAL(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

	h = z::encode_hex(z::sha256("1"));
	BOOST_CHECK_EQUAL(h, "6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b");

	h = z::encode_hex(z::sha256("The SHA (Secure Hash Algorithm) is one of a number of cryptographic hash functions. A cryptographic hash is like a signature for a data set. If you would like to compare two sets of raw data (source of the file, text or similar) it is always better to hash it and compare SHA256 values. It is like the fingerprints of the data. Even if only one symbol is changed the algorithm will produce different hash value. SHA256 algorithm generates an almost-unique, fixed size 256-bit (32-byte) hash. Hash is so called a one way function. This makes it suitable for checking integrity of your data, challenge hash authentication, anti-tamper, digital signatures, blockchain."));
	BOOST_CHECK_EQUAL(h, "ae8bd70b42c2877e6800f3da2800044c8694f201242a484d38bb7941645e8876");
}

BOOST_AUTO_TEST_CASE(crypto_hmac_1)
{
	auto h = z::encode_hex(z::hmac_sha256("The quick brown fox jumps over the lazy dog", "key"));
	BOOST_CHECK_EQUAL(h, "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
	
	h = z::encode_hex(z::hmac_md5("The quick brown fox jumps over the lazy dog", "key"));
	BOOST_CHECK_EQUAL(h, "80070713463e7749b90c2dc24911e275");
}

BOOST_AUTO_TEST_CASE(crypto_pbkdf2)
{
	auto h = z::encode_hex(z::pbkdf2_hmac_sha256("1234", "key", 10, 16));
	BOOST_CHECK_EQUAL(h, "458d81e7a1defc5d0b61708a7dc06233");
}

BOOST_AUTO_TEST_CASE(streambuf_1)
{
	const char s[] = "Hello, world!";

	auto sb = zeep::char_streambuf(s);
	std::istream is(&sb);

	auto len = is.seekg(0, std::ios_base::end).tellg();

	BOOST_CHECK_EQUAL(len, strlen(s));

	is.seekg(0);
	std::vector<char> b(len);
	is.read(b.data(), len);

	BOOST_CHECK_EQUAL(is.tellg(), len);
	BOOST_CHECK_EQUAL(std::string(b.begin(), b.end()), s);
}