//        Copyright Maarten L. Hekkelman, 2019-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// --------------------------------------------------------------------
//
// This file contains an interface to the crypto related routines used
// throughout libzeep.

#pragma once

namespace zeep
{

// --------------------------------------------------------------------
// encoding/decoding

class invalid_base64 : public std::exception
{
  public:
	invalid_base64() {}

	const char* what() const noexcept { return "invalid base64 input"; }
};

std::string encode_base64(const std::string& data, size_t wrap_width = 0);
std::string decode_base64(const std::string& data);

/// The base64url versions are slightly different

std::string encode_base64url(const std::string& data);
std::string decode_base64url(std::string data);

class invalid_hex : public std::exception
{
  public:
	invalid_hex() {}

	const char* what() const noexcept { return "invalid hexadecimal input"; }
};

std::string encode_hex(const std::string& data);
std::string decode_hex(const std::string& data);

// --------------------------------------------------------------------
// random bytes

// return a string containing some random bytes
std::string random_hash();

// --------------------------------------------------------------------
// hashing

std::string md5(const std::string& data);
std::string sha1(const std::string& data);
std::string sha256(const std::string& data);

// --------------------------------------------------------------------
// hmac

std::string hmac_sha1(const std::string& message, const std::string& key);
std::string hmac_sha256(const std::string& message, const std::string& key);
std::string hmac_md5(const std::string& message, const std::string& key);

// --------------------------------------------------------------------
// key derivation based on password (PBKDF2)

/// create password hash according to PBKDF2 with HmacSHA1
std::string pbkdf2_hmac_sha1(const std::string& salt,
	const std::string& password, unsigned iterations, unsigned keyLength);

/// create password hash according to PBKDF2 with HmacSHA256
std::string pbkdf2_hmac_sha256(const std::string& salt,
	const std::string& password, unsigned iterations, unsigned keyLength);

}

