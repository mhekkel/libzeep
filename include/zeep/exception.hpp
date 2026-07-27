// SPDX-FileCopyrightText: Maarten L. Hekkelman, Radboud University 2008-2013.
// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2014-2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

/// \file
/// definition of zeep::exception, base class for exceptions thrown by libzeep

#include "zeep/config.hpp"

#include <exception>
#include <string>

namespace zeep
{

/// \brief base class of the exceptions thrown by libzeep
class exception : public std::exception
{
  public:
	/// \brief Create an exception with the message in \a message
	exception(std::string message)
		: m_message(std::move(message)) {}

	[[nodiscard]] const char* what() const noexcept override { return m_message.c_str(); }

  protected:
	std::string m_message;
};

/// \brief logic error as thrown by libzeep
class logic_exception : public exception
{
  public:
	/// \brief Create an exception with the message in \a message
	logic_exception(std::string message)
		: exception(std::move(message)) {}
};

/// \brief invalid_argument error as thrown by libzeep
class invalid_argument_exception : public exception
{
  public:
	/// \brief Create an exception with the message in \a message
	invalid_argument_exception(std::string message)
		: exception(std::move(message)) {}
};




} // namespace zeep
