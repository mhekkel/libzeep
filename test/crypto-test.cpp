// SPDX-FileCopyrightText: Maarten L. Hekkelman 2026
// SPDX-License-Identifier: BSL-1.0

#include <catch2/catch_test_macros.hpp>

#include <zeep/crypto.hpp>
#include <zeep/streambuf.hpp>

#include <cstring>
#include <istream>
#include <string>
#include <vector>

TEST_CASE("http_base64_1")
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

	CHECK(test == out);

	auto s = zeep::decode_base64(test);

	CHECK(s == in);
}

TEST_CASE("http_base32_1")
{
	using namespace std::literals;

	auto in = R"(Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.)"s;

	auto out = R"(JVQW4IDJOMQGI2LTORUW4Z3VNFZWQZLEFQQG433UEBXW43DZEBRHSIDINFZSA4TFMFZW63RMEBRH
K5BAMJ4SA5DINFZSA43JNZTXK3DBOIQHAYLTONUW63RAMZZG63JAN52GQZLSEBQW42LNMFWHGLBA
O5UGSY3IEBUXGIDBEBWHK43UEBXWMIDUNBSSA3LJNZSCYIDUNBQXIIDCPEQGCIDQMVZHGZLWMVZG
C3TDMUQG6ZRAMRSWY2LHNB2CA2LOEB2GQZJAMNXW45DJNZ2WKZBAMFXGIIDJNZSGKZTBORUWOYLC
NRSSAZ3FNZSXEYLUNFXW4IDPMYQGW3TPO5WGKZDHMUWCAZLYMNSWKZDTEB2GQZJAONUG64TUEB3G
K2DFNVSW4Y3FEBXWMIDBNZ4SAY3BOJXGC3BAOBWGKYLTOVZGKLQ=
)"s;

	auto test = zeep::encode_base32(in, 76);

	CHECK(test == out);

	auto s = zeep::decode_base32(test);

	CHECK(s == in);
}

TEST_CASE("http_base64_2")
{
	using namespace std::literals;

	const std::string tests[] = {
		"1", "12", "123", "1234",
		{ '\0' }, { '\0', '\001' }, { '\0', '\001', '\002' }
	};

	for (const auto &test : tests)
	{
		auto enc = zeep::encode_base64(test, 76);

		auto dec = zeep::decode_base64(enc);

		CHECK(dec == test);
	}
}

TEST_CASE("crypto_md5_1")
{
	auto h = zeep::encode_hex(zeep::md5("1234"));
	CHECK(h == "81dc9bdb52d04dc20036dbd8313ed055");
}

TEST_CASE("crypto_sha1_1")
{
	auto h = zeep::encode_hex(zeep::sha1("The quick brown fox jumps over the lazy dog"));
	CHECK(h == "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

TEST_CASE("crypto_sha256_1")
{
	auto h = zeep::encode_hex(zeep::sha256(""));
	CHECK(h == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

	h = zeep::encode_hex(zeep::sha256("1"));
	CHECK(h == "6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b");

	h = zeep::encode_hex(zeep::sha256("The SHA (Secure Hash Algorithm) is one of a number of cryptographic hash functions. A cryptographic hash is like a signature for a data set. If you would like to compare two sets of raw data (source of the file, text or similar) it is always better to hash it and compare SHA256 values. It is like the fingerprints of the data. Even if only one symbol is changed the algorithm will produce different hash value. SHA256 algorithm generates an almost-unique, fixed size 256-bit (32-byte) hash. Hash is so called a one way function. This makes it suitable for checking integrity of your data, challenge hash authentication, anti-tamper, digital signatures, blockchain."));
	CHECK(h == "ae8bd70b42c2877e6800f3da2800044c8694f201242a484d38bb7941645e8876");
}

TEST_CASE("crypto_hmac_1")
{
	auto h = zeep::encode_hex(zeep::hmac_sha256("The quick brown fox jumps over the lazy dog", "key"));
	CHECK(h == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");

	h = zeep::encode_base64(zeep::hmac_sha1("The quick brown fox jumps over the lazy dog", "key"));
	CHECK(h == "3nybhbi3iqa8ino29wqQcBydtNk=");

	h = zeep::encode_hex(zeep::hmac_sha256("The quick brown fox jumps over the lazy dog", "key"));
	CHECK(h == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");

	h = zeep::encode_hex(zeep::hmac_md5("The quick brown fox jumps over the lazy dog", "key"));
	CHECK(h == "80070713463e7749b90c2dc24911e275");
}

TEST_CASE("crypto_pbkdf2")
{
	auto h = zeep::encode_hex(zeep::pbkdf2_hmac_sha256("1234", "key", 10, 16));
	CHECK(h == "458d81e7a1defc5d0b61708a7dc06233");
}

TEST_CASE("crypto_md5_rfc1321")
{
	using namespace std::literals;

	CHECK(zeep::encode_hex(zeep::md5(""s)) == "d41d8cd98f00b204e9800998ecf8427e");
	CHECK(zeep::encode_hex(zeep::md5("a"s)) == "0cc175b9c0f1b6a831c399e269772661");
	CHECK(zeep::encode_hex(zeep::md5("abc"s)) == "900150983cd24fb0d6963f7d28e17f72");
	CHECK(zeep::encode_hex(zeep::md5("message digest"s)) == "f96b697d7cb7938d525a2f31aaf161d0");
	CHECK(zeep::encode_hex(zeep::md5("abcdefghijklmnopqrstuvwxyz"s)) == "c3fcd3d76192e4007dfb496cca67e13b");
	CHECK(zeep::encode_hex(zeep::md5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"s)) == "d174ab98d277d9f5a5611c2c9f419d9f");
	CHECK(zeep::encode_hex(zeep::md5("12345678901234567890123456789012345678901234567890123456789012345678901234567890"s)) == "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_CASE("crypto_sha1_fips180")
{
	using namespace std::literals;

	CHECK(zeep::encode_hex(zeep::sha1(""s)) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
	CHECK(zeep::encode_hex(zeep::sha1("abc"s)) == "a9993e364706816aba3e25717850c26c9cd0d89d");
	CHECK(zeep::encode_hex(zeep::sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"s)) == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST_CASE("crypto_sha256_fips180")
{
	using namespace std::literals;

	CHECK(zeep::encode_hex(zeep::sha256(""s)) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	CHECK(zeep::encode_hex(zeep::sha256("abc"s)) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	CHECK(zeep::encode_hex(zeep::sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"s)) == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST_CASE("crypto_hmac_md5_rfc2202")
{
	{
		std::string key(16, '\x0b');
		CHECK(zeep::encode_hex(zeep::hmac_md5("Hi There", key)) == "9294727a3638bb1c13f48ef8158bfc9d");
	}

	{
		CHECK(zeep::encode_hex(zeep::hmac_md5("what do ya want for nothing?", "Jefe")) == "750c783e6ab0b503eaa86e310a5db738");
	}

	{
		std::string key(16, '\xaa');
		std::string data(50, '\xdd');
		CHECK(zeep::encode_hex(zeep::hmac_md5(data, key)) == "56be34521d144c88dbb8c733f0e8b3f6");
	}

	{
		std::string key{'\x01','\x02','\x03','\x04','\x05','\x06','\x07','\x08','\x09','\x0a','\x0b','\x0c','\x0d','\x0e','\x0f','\x10','\x11','\x12','\x13','\x14','\x15','\x16','\x17','\x18','\x19'};
		std::string data(50, '\xcd');
		CHECK(zeep::encode_hex(zeep::hmac_md5(data, key)) == "697eaf0aca3a3aea3a75164746ffaa79");
	}
}

TEST_CASE("crypto_hmac_sha1_rfc2202")
{
	{
		std::string key(20, '\x0b');
		CHECK(zeep::encode_hex(zeep::hmac_sha1("Hi There", key)) == "b617318655057264e28bc0b6fb378c8ef146be00");
	}

	{
		CHECK(zeep::encode_hex(zeep::hmac_sha1("what do ya want for nothing?", "Jefe")) == "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
	}

	{
		std::string key(20, '\xaa');
		std::string data(50, '\xdd');
		CHECK(zeep::encode_hex(zeep::hmac_sha1(data, key)) == "125d7342b9ac11cd91a39af48aa17b4f63f175d3");
	}

	{
		std::string key{'\x01','\x02','\x03','\x04','\x05','\x06','\x07','\x08','\x09','\x0a','\x0b','\x0c','\x0d','\x0e','\x0f','\x10','\x11','\x12','\x13','\x14','\x15','\x16','\x17','\x18','\x19'};
		std::string data(50, '\xcd');
		CHECK(zeep::encode_hex(zeep::hmac_sha1(data, key)) == "4c9007f4026250c6bc8414f9bf50c86c2d7235da");
	}
}

TEST_CASE("crypto_hmac_sha256_rfc4231")
{
	{
		std::string key(20, '\x0b');
		CHECK(zeep::encode_hex(zeep::hmac_sha256("Hi There", key)) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
	}

	{
		CHECK(zeep::encode_hex(zeep::hmac_sha256("what do ya want for nothing?", "Jefe")) == "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
	}

	{
		std::string key(20, '\xaa');
		std::string data(50, '\xdd');
		CHECK(zeep::encode_hex(zeep::hmac_sha256(data, key)) == "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");
	}

	{
		std::string key{'\x01','\x02','\x03','\x04','\x05','\x06','\x07','\x08','\x09','\x0a','\x0b','\x0c','\x0d','\x0e','\x0f','\x10','\x11','\x12','\x13','\x14','\x15','\x16','\x17','\x18','\x19'};
		std::string data(50, '\xcd');
		CHECK(zeep::encode_hex(zeep::hmac_sha256(data, key)) == "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");
	}

	{
		std::string key(20, '\x0c');
		CHECK(zeep::encode_hex(zeep::hmac_sha256("Test With Truncation", key)).substr(0, 32) == "a3b6167473100ee06e0c796c2955552b");
	}

	{
		std::string key(131, '\xaa');
		CHECK(zeep::encode_hex(zeep::hmac_sha256("Test Using Larger Than Block-Size Key - Hash Key First", key)) == "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
	}
}

TEST_CASE("crypto_pbkdf2_sha1_rfc6070")
{
	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha1("salt", "password", 1, 20)) == "0c60c80f961f0e71f3a9b524af6012062fe037a6");

	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha1("salt", "password", 2, 20)) == "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957");

	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha1("salt", "password", 4096, 20)) == "4b007901b765489abead49d926f721d065a429c1");

	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha1("saltSALTsaltSALTsaltSALTsaltSALTsalt", "passwordPASSWORDpassword", 4096, 25)) == "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038");
}

TEST_CASE("crypto_pbkdf2_sha256")
{
	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha256("salt", "password", 1, 32)) == "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");

	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha256("salt", "password", 2, 32)) == "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");

	CHECK(zeep::encode_hex(zeep::pbkdf2_hmac_sha256("salt", "password", 4096, 32)) == "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");
}

TEST_CASE("streambuf_1")
{
	const char s[] = "Hello, world!";

	auto sb = zeep::char_streambuf(s);
	std::istream is(&sb);

	auto len = is.seekg(0, std::ios_base::end).tellg();

	CHECK(len == strlen(s));

	is.seekg(0);
	std::vector<char> b(len);
	is.read(b.data(), len);

	CHECK(is.tellg() == len);
	CHECK(std::string(b.begin(), b.end()) == s);
}

TEST_CASE("random_hash produces 16 distinct bytes")
{
	auto h1 = zeep::random_hash();
	auto h2 = zeep::random_hash();

	REQUIRE(h1.size() == 16);
	CHECK(h1 != h2);

	bool all_zero = true;
	for (char c : h1)
		all_zero = all_zero and c == '\0';
	CHECK(not all_zero);
}

TEST_CASE("secure_scrub zeroes contents")
{
	std::string secret = "s3cr3t-password";
	auto size = secret.size();
	zeep::secure_scrub(secret);
	CHECK(secret == std::string(size, '\0'));
	CHECK(secret.size() == size);
}
