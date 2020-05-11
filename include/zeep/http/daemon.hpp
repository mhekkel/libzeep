// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// 
/// Source code specifically for Unix/Linux.
/// Utility routines to build daemon processes

#include <string>
#include <functional>

namespace zeep::http
{

class server;

class daemon
{
  public:

	using server_factory_type = std::function<server*()>;

	daemon(server_factory_type&& factory, const std::string& pid_file,
		const std::string& stdout_log_file, const std::string& stderr_log_file);

	daemon(server_factory_type&& factory, const char* name);

	void set_max_restarts(int nr_of_restarts, int within_nr_of_seconds)
	{
		m_max_restarts = nr_of_restarts;
		m_restart_time_window = within_nr_of_seconds;
	}

	int start(const std::string& address, uint16_t port,
		size_t nr_of_threads, const std::string& run_as_user);

	int stop();
	int status();
	int reload();

	int run_foreground(const std::string& address, uint16_t port);

  private:

	void daemonize();
	void open_log_file();
	bool run_main_loop(const std::string& address, uint16_t port,
		size_t nr_of_threads, const std::string& run_as_user);

	bool pid_is_for_executable();

  private:
	server_factory_type m_factory;
	const std::string m_pid_file, m_stdout_log_file, m_stderr_log_file;

	int m_max_restarts = 5, m_restart_time_window = 10;
};

}
