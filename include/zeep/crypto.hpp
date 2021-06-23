//        Copyright Maarten L. Hekkelman, 2019-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// --------------------------------------------------------------------

#pragma once

#include <zeep/config.hpp>

#include <exception>
#include <string>

/// \file
/// This file contains an interface to the crypto related routines used
/// throughout libzeep.

namespace zeep
{

// --------------------------------------------------------------------
// encoding/decoding

/// \brief Thrown when the input does not contain valid base64 encoded data
class invalid_base64 : public std::exception
{
  public:
	invalid_base64() {}

	const char* what() const noexcept { return "invalid base64 input"; }
};

/// \brief encode \a data in base64 format
///
/// \param data			The string containing data to encode
/// \param wrap_width	If this value is not zero, lines in the output will be wrapped to this width.
std::string encode_base64(std::string_view data, size_t wrap_width = 0);

/// \brief decode data from base64 format, will throw invalid_base64 in case of invalid input
///
/// \param data			The string containing data to decode
std::string decode_base64(std::string_view data);

// The base64url versions are slightly different

/// \brief encode \a data in base64url format (see https://tools.ietf.org/html/rfc4648#section-5)
///
/// \param data			The string containing data to encode
std::string encode_base64url(std::string_view data);

/// \brief decode \a data from base64url format (see https://tools.ietf.org/html/rfc4648#section-5)
///
/// \param data			The string containing data to decode
std::string decode_base64url(std::string data);

// And base32 might be handy as well, RFC 4648 (see https://en.wikipedia.org/wiki/Base32)

/// \brief Thrown when the input does not contain valid base32 encoded data
class invalid_base32 : public std::exception
{
  public:
	invalid_base32() {}

	const char* what() const noexcept { return "invalid base32 input"; }
};

/// \brief encode \a data in base32 format
///
/// \param data			The string containing data to encode
/// \param wrap_width	If this value is not zero, lines in the output will be wrapped to this width.
std::string encode_base32(std::string_view data, size_t wrap_width = 0);

/// \brief decode data from base32 format, will throw invalid_base32 in case of invalid input
///
/// \param data			The string containing data to decode
std::string decode_base32(std::string_view data);

/// \brief Thrown when the input does not contain valid hexadecimal encoded data
class invalid_hex : public std::exception
{
  public:
	invalid_hex() {}

	const char* what() const noexcept { return "invalid hexadecimal input"; }
};

/// \brief encode \a data in hexadecimal format
///
/// \param data			The string containing data to encode
std::string encode_hex(std::string_view data);

/// \brief decode \a data from hexadecimal format
///
/// \param data			The string containing data to decode
std::string decode_hex(std::string_view data);

// --------------------------------------------------------------------
// random bytes

/// \brief return a string containing some random bytes
std::string random_hash();

// --------------------------------------------------------------------
// hashing

/// \brief return the MD5 hash of \a data
std::string md5(std::string_view data);

/// \brief return the SHA1 hash of \a data
std::string sha1(std::string_view data);

/// \brief return the SHA1 hash of \a data
std::string sha1(std::streambuf& data);

/// \brief return the SHA256 hash of \a data
std::string sha256(std::string_view data);

// --------------------------------------------------------------------
// hmac

/// \brief return the HMAC using an MD5 hash of \a message signed with \a key
std::string hmac_md5(std::string_view message, std::string_view key);

/// \brief return the HMAC using an SHA1 hash of \a message signed with \a key
std::string hmac_sha1(std::string_view message, std::string_view key);

/// \brief return the HMAC using an SHA256 hash of \a message signed with \a key
std::string hmac_sha256(std::string_view message, std::string_view key);

// --------------------------------------------------------------------
// key derivation based on password (PBKDF2)

/// \brief create password hash according to PBKDF2 with HmacSHA1
///
/// This algorithm can be used to create keys for symmetric encryption.
/// But you can also use it to store hashed passwords for user authentication.
///
/// \param salt			the salt to use
/// \param password		the password
/// \param iterations	number of iterations, use a value of at least 30000
/// \param keyLength	the requested key length that will be returned
std::string pbkdf2_hmac_sha1(std::string_view salt,
	std::string_view password, unsigned iterations, unsigned keyLength);

/// \brief create password hash according to PBKDF2 with HmacSHA256
///
/// This algorithm can be used to create keys for symmetric encryption.
/// But you can also use it to store hashed passwords for user authentication.
///
/// \param salt			the salt to use
/// \param password		the password
/// \param iterations	number of iterations, use a value of at least 30000
/// \param keyLength	the requested key length that will be returned
std::string pbkdf2_hmac_sha256(std::string_view salt,
	std::string_view password, unsigned iterations, unsigned keyLength);

}

