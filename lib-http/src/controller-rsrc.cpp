// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <zeep/streambuf.hpp>
#include <zeep/http/template-processor.hpp>

#include "mrsrc.h"

namespace ba = boost::algorithm;
namespace fs = std::filesystem;

namespace zeep::http
{

// -----------------------------------------------------------------------

class rsrc_istream : public std::istream
{
  public:

	~rsrc_istream()
	{
		delete m_buffer;
	}

	static rsrc_istream* create(const std::string& file)
	{
		mrsrc::rsrc rsrc(file);
		return rsrc ? new rsrc_istream(new char_streambuf(rsrc.data(), rsrc.size())) : nullptr;
	}

  private:
	rsrc_istream(std::streambuf* buffer)
		: std::istream(buffer)
		, m_buffer(buffer)
	{
	}

	mrsrc::rsrc m_rsrc;
	std::streambuf* m_buffer;
};

rsrc_loader::rsrc_loader(const std::string&)
{
	char exePath[PATH_MAX + 1];
	int r = readlink("/proc/self/exe", exePath, PATH_MAX);
	if (r > 0)
	{
		exePath[r] = 0;
		mRsrcWriteTime = fs::last_write_time(exePath);
	}
}

/// return last_write_time of \a file
fs::file_time_type rsrc_loader::file_time(const std::string& file, std::error_code& ec) noexcept
{
	fs::file_time_type result = {};

	ec = {};
	mrsrc::rsrc rsrc(file);

	if (rsrc)
		result = mRsrcWriteTime;
	else
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	
	return result;
}

// basic loader, returns error in ec if file was not found
std::istream* rsrc_loader::load_file(const std::string& file, std::error_code& ec) noexcept
{
	std::istream* result = rsrc_istream::create(file);

	if (result)
		ec = {};
	else
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	
	return result;
}

} // namespace zeep::http
