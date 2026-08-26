// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2019-2026
// SPDX-License-Identifier: BSL-1.0
//
// --------------------------------------------------------------------

#pragma once

/// \file crypto.hpp
/// This file contains an interface to the crypto related routines used
/// throughout libzeep.

#include <cstddef>
#include <exception>
#include <iosfwd>
#include <string>
#include <string_view>

namespace zeep
{

// --------------------------------------------------------------------
/// \name Encoding / decoding
///@{

/// \brief Thrown when the input does not contain valid base64 encoded data
class invalid_base64 : public std::exception
{
  public:
	invalid_base64() = default;

	[[nodiscard]] const char *what() const noexcept override { return "invalid base64 input"; }
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
	invalid_base32() = default;

	[[nodiscard]] const char *what() const noexcept override { return "invalid base32 input"; }
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
	invalid_hex() = default;

	[[nodiscard]] const char *what() const noexcept override { return "invalid hexadecimal input"; }
};

/// \brief encode \a data in hexadecimal format
///
/// \param data			The string containing data to encode
std::string encode_hex(std::string_view data);

/// \brief decode \a data from hexadecimal format
///
/// \param data			The string containing data to decode
std::string decode_hex(std::string_view data);

///@}

// --------------------------------------------------------------------
/// \name Secure string comparison
///@{

/// \brief compare two strings in constant time

constexpr bool strings_match(std::string_view a, std::string_view b) noexcept
{
	volatile int diff = 0;

	if (a.size() != b.size())
	{
		// lengths differ – but still do a dummy comparison to avoid
		// leaking the length difference via timing
		for (size_t i = 0; i < std::max(a.size(), b.size()); ++i)
			diff |= (i < a.size() ? a[i] : 0) xor (i < b.size() ? b[i] : 0);
		diff = 1;
	}
	else
	{
		for (size_t i = 0; i < a.size(); ++i)
			diff |= static_cast<unsigned char>(a[i]) xor static_cast<unsigned char>(b[i]);
	}

	return diff == 0;
}

///@}

// --------------------------------------------------------------------
/// \name Secret scrubbing
///@{

/// \brief Securely zero the contents of a string that held a secret.
///
/// This best-effort wipe reduces the window in which a password, derived key
/// or HMAC key remains in heap memory. Note that `std::string` may leave
/// unscrubbed copies behind on reallocation, so this is not a complete
/// guarantee; use it in addition to storing secrets in dedicated buffers.
/// \param s  The string whose contents should be zeroed
inline void secure_scrub(std::string &s) noexcept
{
	volatile char *p = reinterpret_cast<volatile char *>(s.data());
	for (size_t i = 0; i < s.size(); ++i)
		p[i] = 0;
}

///@}

// --------------------------------------------------------------------
/// \name Random bytes
///@{

/// \brief return a string containing some random bytes
std::string random_hash();

///@}

// --------------------------------------------------------------------
/// \name Hashing
///@{

/// \brief return the MD5 hash of \a data
std::string md5(std::string_view data);

/// \brief return the SHA1 hash of \a data (string view overload)
std::string sha1(std::string_view data);

/// \brief return the SHA1 hash of \a data (streambuf overload)
std::string sha1(std::streambuf &data);

/// \brief return the SHA256 hash of \a data
std::string sha256(std::string_view data);

///@}

// --------------------------------------------------------------------
/// \name HMAC
///@{

/// \brief return the HMAC using an MD5 hash of \a message signed with \a key
std::string hmac_md5(std::string_view message, std::string_view key);

/// \brief return the HMAC using an SHA1 hash of \a message signed with \a key
std::string hmac_sha1(std::string_view message, std::string_view key);

/// \brief return the HMAC using an SHA256 hash of \a message signed with \a key
std::string hmac_sha256(std::string_view message, std::string_view key);

///@}

// --------------------------------------------------------------------
/// \name Key derivation (PBKDF2)
///@{

/// \brief create password hash according to PBKDF2 with HmacSHA1

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

///@}

} // namespace zeep
