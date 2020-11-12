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
	mrsrc::rsrc resource(file);

	std::istream* result = nullptr;
	ec = {};

	if (resource)
		result = new mrsrc::istream(resource);
	else
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
	
	return result;
}

} // namespace zeep::http
