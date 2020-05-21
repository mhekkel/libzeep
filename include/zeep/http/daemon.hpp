//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// 
/// Source code specifically for Unix/Linux.
/// Utility routines to build daemon processes

#include <zeep/config.hpp>

#include <string>
#include <functional>

namespace zeep::http
{

class server;

/// \brief A class to create daemon processes easily
///
/// In UNIX a daemon is a process that runs in the background.
/// In the case of libzeep this is of course serving HTTP requests.
/// stderr and stdout are captured and written to the log files
/// specified and a process ID is store in the pid file which
/// allows checking the status of a running daemon.

class daemon
{
  public:

	/// \brief The factory for creating server instances.
	using server_factory_type = std::function<server*()>;

	/// \brief constructor with separately specified files
	///
	/// \param factory			The function object that creates server instances
	/// \param pid_file			The file that will contain the process ID, usually in /var/run/<process_name>
	/// \param stdout_log_file	The file that will contain the stdout log, usually in /var/log/<process_name>/access.log
	/// \param stderr_log_file	The file that will contain the stderr log, usually in /var/log/<process_name>/error.log
	daemon(server_factory_type&& factory, const std::string& pid_file,
		const std::string& stdout_log_file, const std::string& stderr_log_file);

	/// \brief constructor with default files
	///
	/// \param factory			The function object that creates server instances
	/// \param name				The _process name_ to use, will be used to form default file locations
	daemon(server_factory_type&& factory, const char* name);

	/// \brief Avoid excessive automatic restart due to failing to start up
	///
	/// \param nr_of_restarts			The max number of attempts to take to start up a daemon process
	/// \param within_nr_of_seconds		The restart counter will only consider a failed restart if it fails
	///                                 starting up within this period of time.
	void set_max_restarts(int nr_of_restarts, int within_nr_of_seconds)
	{
		m_max_restarts = nr_of_restarts;
		m_restart_time_window = within_nr_of_seconds;
	}

	/// \brief Start the daemon, forking off in the background
	///
	/// \param address				The address to bind to
	/// \param port					The port number to bind to
	/// \param nr_of_threads		The number of threads to pass to the server class
	/// \param run_as_user			The user to run the forked process. Daemons are usually
	///								started as root and should drop their privileges as soon
	///								as possible.
	int start(const std::string& address, uint16_t port,
		size_t nr_of_threads, const std::string& run_as_user);

	/// \brief Stop a running daemon process. Returns 0 in case of successfully stopping a process.
	int stop();

	/// \brief Returns 0 if the daemon is running
	int status();

	/// \brief Force the running daemon to restart
	int reload();

	/// \brief Run the server without forking to the background
	///
	/// For debugging purposes it is sometimes useful to start a server
	/// without forking so you can see the stdout and stderr. Often this
	/// is done by adding a --no-daemon flag to the program options.
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
