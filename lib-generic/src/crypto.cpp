//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <climits>
#include <cstring>

#include <vector>
#include <iostream>


#include <boost/random/random_device.hpp>

#include <zeep/crypto.hpp>

namespace zeep
{

// --------------------------------------------------------------------
// encoding/decoding

const char kCharTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const uint8_t kIndexTable[128] = {
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	62, // +
	128, // not used
	128, // not used
	128, // not used
	63, // /
	52, // 0
	53, // 1
	54, // 2
	55, // 3
	56, // 4
	57, // 5
	58, // 6
	59, // 7
	60, // 8
	61, // 9
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	0, // A
	1, // B
	2, // C
	3, // D
	4, // E
	5, // F
	6, // G
	7, // H
	8, // I
	9, // J
	10, // K
	11, // L
	12, // M
	13, // N
	14, // O
	15, // P
	16, // Q
	17, // R
	18, // S
	19, // T
	20, // U
	21, // V
	22, // W
	23, // X
	24, // Y
	25, // Z
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	26, // a
	27, // b
	28, // c
	29, // d
	30, // e
	31, // f
	32, // g
	33, // h
	34, // i
	35, // j
	36, // k
	37, // l
	38, // m
	39, // n
	40, // o
	41, // p
	42, // q
	43, // r
	44, // s
	45, // t
	46, // u
	47, // v
	48, // w
	49, // x
	50, // y
	51, // z
	128, // not used
	128, // not used
	128, // not used
	128, // not used
	128, // not used
};

inline size_t sextet(char ch)
{
	if (ch < '+' or ch > 'z')
		throw invalid_base64();

	return kIndexTable[static_cast<uint8_t>(ch)];
}

std::string encode_base64(const std::string& data, size_t wrap_width)
{
	std::string::size_type n = data.length();
	std::string::size_type m = 4 * (n / 3);
	if (n % 3)
		m += 4;

	if (wrap_width != 0)
		m += (m / wrap_width) + 1;

	std::string result;
	result.reserve(m);

	auto ch = data.begin();
	int l = 0;

	while (n > 0)
	{
		char s[4] = { '=', '=', '=', '=' };

		switch (n)
		{
			case 1:
			{
				uint8_t i = *ch++;
				s[0] = kCharTable[i >> 2];
				s[1] = kCharTable[(i << 4) bitand 0x03f];

				n -= 1;
				break;
			}

			case 2:
			{
				uint8_t i1 = *ch++;
				uint8_t i2 = *ch++;

				s[0] = kCharTable[i1 >> 2];
				s[1] = kCharTable[(i1 << 4 bitor i2 >> 4) bitand 0x03f];
				s[2] = kCharTable[(i2 << 2) bitand 0x03f];

				n -= 2;
				break;
			}

			default:
			{
				uint8_t i1 = *ch++;
				uint8_t i2 = *ch++;
				uint8_t i3 = *ch++;

				s[0] = kCharTable[i1 >> 2];
				s[1] = kCharTable[(i1 << 4 bitor i2 >> 4) bitand 0x03f];
				s[2] = kCharTable[(i2 << 2 bitor i3 >> 6) bitand 0x03f];
				s[3] = kCharTable[i3 bitand 0x03f];

				n -= 3;
				break;
			}
		}

		if (wrap_width == 0)
			result.append(s, s + 4);
		else
		{
			for (int i = 0; i < 4; ++i)
			{
				if (l == wrap_width)
				{
					result.append(1, '\n');
					l = 0;
				}

				result.append(1, s[i]);
				++l;
			}
		}
	}

	if (wrap_width != 0)
		result.append(1, '\n');

	assert(result.length() == m);

	return result;
}

std::string decode_base64(const std::string& data)
{
	size_t n = data.length();
	size_t m = 3 * (n / 4);

	std::string result;
	result.reserve(m);

	auto i = data.begin();

	while (i != data.end())
	{
		uint8_t sxt[4] = {};
		int b = 0, c = 3;

		while (b < 4)
		{
			if (i == data.end())
				break;

			char ch = *i++;

			switch (ch)
			{
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					break;
				
				case '=':
					if (b == 2 and *i++ == '=')
					{
						c = 1;
						b = 4;
					}
					else if (b == 3)
					{
						c = 2;
						b = 4;
					}
					else
						throw invalid_base64();
					break;
				
				default:
					sxt[b] = sextet(ch);
					++b;
					break;
			}
		}

		if (b == 4)
		{
			result.append(1, static_cast<char>(sxt[0] << 2 bitor sxt[1] >> 4));
			if (c >= 2)
				result.append(1, static_cast<char>(sxt[1] << 4 bitor sxt[2] >> 2));
			if (c == 3)
				result.append(1, static_cast<char>(sxt[2] << 6 bitor sxt[3]));
		}
		else if (b != 0)
			throw invalid_base64();
	}

	return result;
}

std::string encode_base64url(const std::string& data)
{
	std::string result = encode_base64(data);

	while (not result.empty() and result.back() == '=')
		result.pop_back();
	
	for (auto p = result.find('+'); p != std::string::npos; p = result.find('+', p + 1))
		result.replace(p, 1, 1, '-');
	for (auto p = result.find('/'); p != std::string::npos; p = result.find('/', p + 1))
		result.replace(p, 1, 1, '_');
	
	return result;
}

std::string decode_base64url(std::string data)
{
	for (auto p = data.find('-'); p != std::string::npos; p = data.find('-', p + 1))
		data.replace(p, 1, 1, '+');
	for (auto p = data.find('_'); p != std::string::npos; p = data.find('_', p + 1))
		data.replace(p, 1, 1, '/');

	switch (data.length() % 4)
	{
		case 0:	break;
		case 2: data.append(2, '='); break;
		case 3: data.append(1, '='); break;
		default: throw invalid_base64();
	}

	return decode_base64(data);
}

// --------------------------------------------------------------------
// hex

std::string encode_hex(const std::string& data)
{
	// convert to hex
	const char kHexChars[] = "0123456789abcdef";
	std::string result;
	result.reserve(data.length() * 2);
	
	for (uint8_t b: data)
	{
		result += kHexChars[b >> 4];
		result += kHexChars[b bitand 0x0f];
	}
	
	return result;
}

std::string decode_hex(const std::string& data)
{
	if (data.length() % 2 == 1)
		throw invalid_hex();

	std::string result;
	result.reserve(data.length() / 2);

	for (std::string::size_type i = 0; i < data.length(); i += 2)
	{
		uint8_t n[2] = {};

		for (int j = 0; j < 2; ++j)
		{
			auto ch = data[i + j];
			if (ch >= '0' and ch <= '9')
				n[i] = ch - '0';
			else if (ch >= 'a' and ch <= 'f')
				n[i] = ch - 'a' + 10;
			else if (ch >= 'A' and ch <= 'F')
				n[i] = ch - 'A' + 10;
			else
				throw invalid_hex();
		}

		result.push_back(static_cast<char>(n[0] << 4 bitor n[1]));
	}

	return result;
}

// --------------------------------------------------------------------
// random

std::string random_hash()
{
	boost::random::random_device rng;

	union {
		uint32_t data[4];
		char s[4 * 4];
	} v = { { rng(), rng(), rng(), rng() } };

	return { v.s, v.s + sizeof(v) };
}

// --------------------------------------------------------------------
// hashes



// --------------------------------------------------------------------

static inline uint32_t rotl32 (uint32_t n, unsigned int c)
{
	const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);  // assumes width is a power of 2.

	// assert ( (c<=mask) &&"rotate by type width or more");
	c &= mask;
	return (n<<c) bitor (n>>( (-c)&mask ));
}

static inline uint32_t rotr32 (uint32_t n, unsigned int c)
{
	const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);

	// assert ( (c<=mask) &&"rotate by type width or more");
	c &= mask;
	return (n>>c) bitor (n<<( (-c)&mask ));
}

// --------------------------------------------------------------------

struct hash_impl
{
	virtual ~hash_impl() {}

	virtual void write_bit_length(uint64_t l, uint8_t* b) = 0;
	virtual void transform(const uint8_t* data) = 0;
	virtual std::string final() = 0;
};

// --------------------------------------------------------------------

#define F1(x,y,z)	(z xor (x bitand (y xor z)))
#define F2(x,y,z)	F1(z, x, y)
#define F3(x,y,z)	(x xor y xor z)
#define	F4(x,y,z)	(y xor ( x bitor compl z))

#define STEP(F,w,x,y,z,data,s) 	\
	w += F(x, y, z) + data;		\
	w = rotl32(w, s); \
	w += x;

struct md5_hash_impl : public hash_impl
{
	using word_type = uint32_t;
	static const size_t word_count = 4;
	static const size_t block_size = 64;
	static const size_t digest_size = word_count * sizeof(word_type);

	word_type	m_h[word_count];

	virtual void init()
	{
		m_h[0] = 0x67452301;
		m_h[1] = 0xefcdab89;
		m_h[2] = 0x98badcfe;
		m_h[3] = 0x10325476;
	}

	virtual void write_bit_length(uint64_t l, uint8_t* p)
	{
		for (int i = 0; i < 8; ++i)
			*p++ = static_cast<uint8_t>((l >> (i * 8)));
	}

	virtual void transform(const uint8_t* data)
	{
		uint32_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3];
		uint32_t in[16];
		
		for (int i = 0; i < 16; ++i)
		{
			in[i] = static_cast<uint32_t>(data[0]) <<  0 |
					static_cast<uint32_t>(data[1]) <<  8 |
					static_cast<uint32_t>(data[2]) << 16 |
					static_cast<uint32_t>(data[3]) << 24;
			data += 4;
		}
		
		STEP(F1, a, b, c, d, in[ 0] + 0xd76aa478,  7);
		STEP(F1, d, a, b, c, in[ 1] + 0xe8c7b756, 12);
		STEP(F1, c, d, a, b, in[ 2] + 0x242070db, 17);
		STEP(F1, b, c, d, a, in[ 3] + 0xc1bdceee, 22);
		STEP(F1, a, b, c, d, in[ 4] + 0xf57c0faf,  7);
		STEP(F1, d, a, b, c, in[ 5] + 0x4787c62a, 12);
		STEP(F1, c, d, a, b, in[ 6] + 0xa8304613, 17);
		STEP(F1, b, c, d, a, in[ 7] + 0xfd469501, 22);
		STEP(F1, a, b, c, d, in[ 8] + 0x698098d8,  7);
		STEP(F1, d, a, b, c, in[ 9] + 0x8b44f7af, 12);
		STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
		STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
		STEP(F1, a, b, c, d, in[12] + 0x6b901122,  7);
		STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
		STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
		STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

		STEP(F2, a, b, c, d, in[ 1] + 0xf61e2562,  5);
		STEP(F2, d, a, b, c, in[ 6] + 0xc040b340,  9);
		STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
		STEP(F2, b, c, d, a, in[ 0] + 0xe9b6c7aa, 20);
		STEP(F2, a, b, c, d, in[ 5] + 0xd62f105d,  5);
		STEP(F2, d, a, b, c, in[10] + 0x02441453,  9);
		STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
		STEP(F2, b, c, d, a, in[ 4] + 0xe7d3fbc8, 20);
		STEP(F2, a, b, c, d, in[ 9] + 0x21e1cde6,  5);
		STEP(F2, d, a, b, c, in[14] + 0xc33707d6,  9);
		STEP(F2, c, d, a, b, in[ 3] + 0xf4d50d87, 14);
		STEP(F2, b, c, d, a, in[ 8] + 0x455a14ed, 20);
		STEP(F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
		STEP(F2, d, a, b, c, in[ 2] + 0xfcefa3f8,  9);
		STEP(F2, c, d, a, b, in[ 7] + 0x676f02d9, 14);
		STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

		STEP(F3, a, b, c, d, in[ 5] + 0xfffa3942,  4);
		STEP(F3, d, a, b, c, in[ 8] + 0x8771f681, 11);
		STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
		STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
		STEP(F3, a, b, c, d, in[ 1] + 0xa4beea44,  4);
		STEP(F3, d, a, b, c, in[ 4] + 0x4bdecfa9, 11);
		STEP(F3, c, d, a, b, in[ 7] + 0xf6bb4b60, 16);
		STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
		STEP(F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
		STEP(F3, d, a, b, c, in[ 0] + 0xeaa127fa, 11);
		STEP(F3, c, d, a, b, in[ 3] + 0xd4ef3085, 16);
		STEP(F3, b, c, d, a, in[ 6] + 0x04881d05, 23);
		STEP(F3, a, b, c, d, in[ 9] + 0xd9d4d039,  4);
		STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
		STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
		STEP(F3, b, c, d, a, in[ 2] + 0xc4ac5665, 23);

		STEP(F4, a, b, c, d, in[ 0] + 0xf4292244,  6);
		STEP(F4, d, a, b, c, in[ 7] + 0x432aff97, 10);
		STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
		STEP(F4, b, c, d, a, in[ 5] + 0xfc93a039, 21);
		STEP(F4, a, b, c, d, in[12] + 0x655b59c3,  6);
		STEP(F4, d, a, b, c, in[ 3] + 0x8f0ccc92, 10);
		STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
		STEP(F4, b, c, d, a, in[ 1] + 0x85845dd1, 21);
		STEP(F4, a, b, c, d, in[ 8] + 0x6fa87e4f,  6);
		STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
		STEP(F4, c, d, a, b, in[ 6] + 0xa3014314, 15);
		STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
		STEP(F4, a, b, c, d, in[ 4] + 0xf7537e82,  6);
		STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
		STEP(F4, c, d, a, b, in[ 2] + 0x2ad7d2bb, 15);
		STEP(F4, b, c, d, a, in[ 9] + 0xeb86d391, 21);
		
		m_h[0] += a;
		m_h[1] += b;
		m_h[2] += c;
		m_h[3] += d;		
	}

	virtual std::string final()
	{
		std::string result;
		result.reserve(digest_size);
		
		for (int i = 0; i < word_count; ++i)
		{
			for (int j = 0; j < sizeof(word_type); ++j)
			{
				uint8_t b = m_h[i] >> (j * 8);
				result.push_back(b);
			}
		}
		
		return result;
	}
};

// --------------------------------------------------------------------

struct sha1_hash_impl : public hash_impl
{
	using word_type = uint32_t;
	static const size_t word_count = 5;
	static const size_t block_size = 64;
	static const size_t digest_size = word_count * sizeof(word_type);

	word_type	m_h[word_count];

	virtual void init()
	{
		m_h[0] = 0x67452301;
		m_h[1] = 0xEFCDAB89;
		m_h[2] = 0x98BADCFE;
		m_h[3] = 0x10325476;
		m_h[4] = 0xC3D2E1F0;
	}

	virtual void write_bit_length(uint64_t l, uint8_t* b)
	{
		b[0] = l >> 56;
		b[1] = l >> 48;
		b[2] = l >> 40;
		b[3] = l >> 32;
		b[4] = l >> 24;
		b[5] = l >> 16;
		b[6] = l >>  8;
		b[7] = l >>  0;
	}

	virtual void transform(const uint8_t* data)
	{
		union {
			uint8_t s[64];
			uint32_t w[80];
		} w;

		auto p = data;
		for (size_t i = 0; i < 16; ++i)
		{
			w.s[i * 4 + 3] = *p++;
			w.s[i * 4 + 2] = *p++;
			w.s[i * 4 + 1] = *p++;
			w.s[i * 4 + 0] = *p++;
		}

		for (size_t i = 16; i < 80; ++i)
			w.w[i] = rotl32(w.w[i - 3] xor w.w[i - 8] xor w.w[i - 14] xor w.w[i - 16], 1);

		word_type wv[word_count];

		for (int i = 0; i < word_count; ++i)
			wv[i] = m_h[i];

		for (size_t i = 0; i < 80; ++i)
		{
			uint32_t f, k;
			if (i < 20)
			{
				f = (wv[1] bitand wv[2]) bitor ((compl wv[1]) bitand wv[3]);
				k = 0x5A827999;
			}
			else if (i < 40)
			{
	            f = wv[1] xor wv[2] xor wv[3];
	            k = 0x6ED9EBA1;
			}
			else if (i < 60)
			{
	            f = (wv[1] bitand wv[2]) bitor (wv[1] bitand wv[3]) bitor (wv[2] bitand wv[3]);
	            k = 0x8F1BBCDC;
			}
			else
			{
				f = wv[1] xor wv[2] xor wv[3];
				k = 0xCA62C1D6;
			}

			uint32_t t = rotl32(wv[0], 5) + f + wv[4] + k + w.w[i];

			wv[4] = wv[3];
			wv[3] = wv[2];
			wv[2] = rotl32(wv[1], 30);
			wv[1] = wv[0];
			wv[0] = t;
		}

		for (int i = 0; i < word_count; ++i)
			m_h[i] += wv[i];
	}

	virtual std::string final()
	{
		std::string result(digest_size, '\0');

		auto s = result.begin();
		for (int i = 0; i < word_count; ++i)
		{
			*s++ = static_cast<char>(m_h[i] >> 24);
			*s++ = static_cast<char>(m_h[i] >> 16);
			*s++ = static_cast<char>(m_h[i] >>  8);
			*s++ = static_cast<char>(m_h[i] >>  0);
		}

		return result;
	}
};

// --------------------------------------------------------------------

struct sha256_hash_impl : public hash_impl
{
	using word_type = uint32_t;
	static const size_t word_count = 8;
	static const size_t block_size = 64;
	static const size_t digest_size = word_count * sizeof(word_type);

	word_type	m_h[word_count];

	virtual void init()
	{
		m_h[0] = 0x6a09e667;
		m_h[1] = 0xbb67ae85;
		m_h[2] = 0x3c6ef372;
		m_h[3] = 0xa54ff53a;
		m_h[4] = 0x510e527f;
		m_h[5] = 0x9b05688c;
		m_h[6] = 0x1f83d9ab;
		m_h[7] = 0x5be0cd19;
	}

	virtual void write_bit_length(uint64_t l, uint8_t* b)
	{
		b[0] = l >> 56;
		b[1] = l >> 48;
		b[2] = l >> 40;
		b[3] = l >> 32;
		b[4] = l >> 24;
		b[5] = l >> 16;
		b[6] = l >>  8;
		b[7] = l >>  0;
	}

	virtual void transform(const uint8_t* data)
	{
		static const uint32_t k[] = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
		};

		word_type wv[word_count];
		for (int i = 0; i < word_count; ++i)
			wv[i] = m_h[i];

		union {
			uint8_t s[64];
			uint32_t w[64];
		} w;

		auto p = data;
		for (size_t i = 0; i < 16; ++i)
		{
			w.s[i * 4 + 3] = *p++;
			w.s[i * 4 + 2] = *p++;
			w.s[i * 4 + 1] = *p++;
			w.s[i * 4 + 0] = *p++;
		}

		for (size_t i = 16; i < 64; ++i)
		{
			auto s0 = rotr32(w.w[i - 15], 7) xor rotr32(w.w[i - 15], 18) xor (w.w[i - 15] >> 3);
			auto s1 = rotr32(w.w[i - 2], 17) xor rotr32(w.w[i - 2], 19) xor (w.w[i - 2] >> 10);
			w.w[i] = w.w[i - 16] + s0 + w.w[i - 7] + s1;
		}

		for (size_t i = 0; i < 64; ++i)
		{
			uint32_t S1 = rotr32(wv[4], 6) xor rotr32(wv[4], 11) xor rotr32(wv[4], 25);
			uint32_t ch = (wv[4] bitand wv[5]) xor (compl wv[4] bitand wv[6]);
			uint32_t t1 = wv[7] + S1 + ch + k[i] + w.w[i];
			uint32_t S0 = rotr32(wv[0], 2) xor rotr32(wv[0], 13) xor rotr32(wv[0], 22);
			uint32_t maj = (wv[0] bitand wv[1]) xor (wv[0] bitand wv[2]) xor (wv[1] bitand wv[2]);
			uint32_t t2 = S0 + maj;

			wv[7] = wv[6];
			wv[6] = wv[5];
			wv[5] = wv[4];
			wv[4] = wv[3] + t1;
			wv[3] = wv[2];
			wv[2] = wv[1];
			wv[1] = wv[0];
			wv[0] = t1 + t2;
		}

		for (int i = 0; i < word_count; ++i)
			m_h[i] += wv[i];
	}

	virtual std::string final()
	{
		std::string result(digest_size, '\0');

		auto s = result.begin();
		for (int i = 0; i < word_count; ++i)
		{
			*s++ = static_cast<char>(m_h[i] >> 24);
			*s++ = static_cast<char>(m_h[i] >> 16);
			*s++ = static_cast<char>(m_h[i] >>  8);
			*s++ = static_cast<char>(m_h[i] >>  0);
		}

		return result;
	}
};

// --------------------------------------------------------------------

template<typename I>
class hash_base : public I
{
  public:
	using word_type = typename I::word_type;
	static const size_t word_count = I::word_count;
	static const size_t block_size = I::block_size;
	static const size_t digest_size = I::digest_size;

	hash_base()
	{
		init();
	}

	void init()
	{
		I::init();

		m_data_length = 0;
		m_bit_length = 0;
	}

	void update(const std::string& data);
	
	using I::transform;
	std::string final();

  private:
	uint8_t		m_data[block_size];
	uint32_t	m_data_length;
	int64_t		m_bit_length;
};

template<typename I>
void hash_base<I>::update(const std::string& data)
{
	auto length = data.length();
	m_bit_length += length * 8;
	
	const uint8_t* p = reinterpret_cast<const uint8_t*>(data.data());
	
	if (m_data_length > 0)
	{
		uint32_t n = block_size - m_data_length;
		if (n > length)
			n = static_cast<uint32_t>(length);
		
		memcpy(m_data + m_data_length, p, n);
		m_data_length += n;
		
		if (m_data_length == block_size)
		{
			transform(m_data);
			m_data_length = 0;
		}
		
		p += n;
		length -= n;
	}
	
	while (length >= block_size)
	{
		transform(p);
		p += block_size;
		length -= block_size;
	}
	
	if (length > 0)
	{
		memcpy(m_data, p, length);
		m_data_length += static_cast<uint32_t>(length);
	}
}

template<typename I>
std::string hash_base<I>::final()
{
	m_data[m_data_length] = 0x80;
	++m_data_length;
	std::fill(m_data + m_data_length, m_data + block_size, 0);
	
	if (block_size - m_data_length < 8)
	{
		transform(m_data);
		std::fill(m_data, m_data + block_size - 8, 0);
	}

	I::write_bit_length(m_bit_length, m_data + block_size - 8);
	
	transform(m_data);
	std::fill(m_data, m_data + block_size, 0);
	
	auto result = I::final();
	init();
	return result;
}

using MD5 = hash_base<md5_hash_impl>;
using SHA1 = hash_base<sha1_hash_impl>;
using SHA256 = hash_base<sha256_hash_impl>;

// --------------------------------------------------------------------

std::string sha1(const std::string& data)
{
	SHA1 h;
	h.init();
	h.update(data);
	return h.final();
}

std::string sha256(const std::string& data)
{
	SHA256 h;
	h.init();
	h.update(data);
	return h.final();
}

std::string md5(const std::string& data)
{
	MD5 h;
	h.init();
	h.update(data);
	return h.final();
}

// --------------------------------------------------------------------
// hmac

template<typename H>
class HMAC
{
  public:
	static const size_t block_size = H::block_size;
	static const size_t digest_size = H::digest_size;

	HMAC(std::string key)
		: m_ipad(block_size, '\x36'), m_opad(block_size, '\x5c')
	{
		if (key.length() > block_size)
		{
			H keyHash;
			keyHash.update(key);
			key = keyHash.final();
		}

		assert(key.length() < block_size);

		for (int i = 0; i < key.length(); ++i)
		{
			m_opad[i] ^= key[i];
			m_ipad[i] ^= key[i];
		}
	}

	HMAC& update(const std::string& data)
	{
		if (not m_inner_updated)
		{
			m_inner.update(m_ipad);
			m_inner_updated = true;
		}

		m_inner.update(data);
		return *this;
	}

	std::string final()
	{
		H outer;
		outer.update(m_opad);
		outer.update(m_inner.final());
		m_inner_updated = false;
		return outer.final();
	}

  private:
	std::string m_ipad, m_opad;
	bool m_inner_updated = false;
	H m_inner;
};

std::string hmac_sha1(const std::string& message, const std::string& key)
{
	return HMAC<SHA1>(key).update(message).final();
}

std::string hmac_sha256(const std::string& message, const std::string& key)
{
	return HMAC<SHA256>(key).update(message).final();
}

std::string hmac_md5(const std::string& message, const std::string& key)
{
	return HMAC<MD5>(key).update(message).final();
}

// --------------------------------------------------------------------
// password/key derivation

template<typename HMAC>
std::string pbkdf2(const std::string& salt,
	const std::string& password, unsigned iterations, unsigned keyLength)
{
	std::string result;

	HMAC hmac(password);

	uint32_t i = 1;
	while (result.length() < keyLength)
	{
		hmac.update(salt);
		for (int j = 0; j < 4; ++j)
		{
			char b = static_cast<char>(i >> ((3 - j) * CHAR_BIT));
			hmac.update(std::string{ b });
		}
		auto derived = hmac.final();
		auto buffer = derived;

		for (unsigned c = 1; c < iterations; ++c)
		{
			buffer = hmac.update(buffer).final();

			for (int i = 0; i < buffer.length(); ++i)
				derived[i] ^= buffer[i];
		}
		
		result.append(derived);
		++i;
	}

	if (result.length() > keyLength)
		result.erase(result.begin() + keyLength, result.end());

	return result;
}

/// create password hash according to PBKDF2 with HmacSHA1
std::string pbkdf2_hmac_sha1(const std::string& salt,
	const std::string& password, unsigned iterations, unsigned keyLength)
{
	return pbkdf2<HMAC<SHA1>>(salt, password, iterations, keyLength);
}

/// create password hash according to PBKDF2 with HmacSHA256
std::string pbkdf2_hmac_sha256(const std::string& salt,
	const std::string& password, unsigned iterations, unsigned keyLength)
{
	return pbkdf2<HMAC<SHA256>>(salt, password, iterations, keyLength);
}

}