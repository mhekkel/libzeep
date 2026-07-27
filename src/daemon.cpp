// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2026
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// Source code specifically for Unix/Linux
// Utility routines to build daemon processes

#include "zeep/http/daemon.hpp"

#include "signals.hpp"
#include "zeep/config.hpp"
#include "zeep/exception.hpp"
#include "zeep/http/asio.hpp"
#include "zeep/http/server.hpp"
#include "zeep/unicode-support.hpp"

#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#ifndef _WIN32
# include <grp.h>
# include <pwd.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace zeep::http
{

#if __APPLE__
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
	return ::getgrouplist(user, (int)group, (int *)groups, ngroups);
}
#endif

daemon::daemon(server_factory_type &&factory, std::string pid_file,
	std::string stdout_log_file, std::string stderr_log_file)
	: m_factory(std::move(factory))
	, m_pid_file(std::move(pid_file))
	, m_stdout_log_file(std::move(stdout_log_file))
	, m_stderr_log_file(std::move(stderr_log_file))
{
}

daemon::daemon(server_factory_type &&factory, const std::string &name)
	: daemon(std::move(factory), "/var/run/" + name,
		  "/var/log/" + name + "/access.log", "/var/log/" + name + "/error.log")
{
}

int daemon::run_foreground(std::string_view address, uint16_t port)
{
	asio_ns::io_context io_context;
	asio_ns::ip::tcp::endpoint endpoint;

	asio_system_ns::error_code ec;
	auto addr = asio_ns::ip::make_address(address, ec);
	if (not ec)
		endpoint = asio_ns::ip::tcp::endpoint(addr, port);
	else
	{
		asio_ns::ip::tcp::resolver resolver(io_context);
		for (auto &ep : resolver.resolve(address, std::to_string(port)))
		{
			endpoint = ep;
			break;
		}
	}

	asio_ns::ip::tcp::acceptor acceptor(io_context);
	acceptor.open(endpoint.protocol());
	acceptor.set_option(asio_ns::ip::tcp::acceptor::reuse_address(true));
	if (acceptor.bind(endpoint, ec))
		throw exception(std::string("Is server running already? ") + ec.message());
	acceptor.listen();

	acceptor.close();

	signal_catcher sc;
	sc.block();

	std::unique_ptr<basic_server> s(m_factory());
	s->bind(address, port);
	std::thread t([s = s.get()]
		{ s->run(1); });

	sc.unblock();

	sc.wait();

	s->stop();

	if (t.joinable())
		t.join();

	return 0;
}

#if HTTP_HAS_UNIX_DAEMON

int daemon::start(std::string_view address, uint16_t port, int nr_of_threads, const std::string &run_as_user)
{
	using namespace std::literals;

	int result = 0;

	if (pid_is_for_executable())
	{
		std::clog << "Server is already running.\n";
		result = 1;
	}
	else
	{
		std::error_code ec;

		if (fs::exists(m_pid_file, ec))
			fs::remove(m_pid_file, ec);

		fs::path pidDir = fs::path(m_pid_file).parent_path();
		if (not fs::is_directory(pidDir, ec))
			fs::create_directories(pidDir, ec);

		if (ec)
			std::clog << "Creating directory for pid file failed: " << ec.message() << '\n';

		fs::path outLogDir = fs::path(m_stdout_log_file).parent_path();
		if (not fs::is_directory(outLogDir, ec))
			fs::create_directories(outLogDir, ec);

		if (ec)
			std::clog << "Creating directory " << outLogDir << " for log files failed: " << ec.message() << '\n';

		fs::path errLogDir = fs::path(m_stderr_log_file).parent_path();
		if (not fs::is_directory(errLogDir, ec))
			fs::create_directories(errLogDir, ec);

		if (ec)
			std::clog << "Creating directory " << outLogDir << " for log files failed: " << ec.message() << '\n';

		try
		{
			asio_ns::io_context io_context;

			asio_ns::ip::tcp::endpoint endpoint;
			try
			{
				endpoint = asio_ns::ip::tcp::endpoint(asio_ns::ip::make_address(address), port);
			}
			catch (const std::exception &e)
			{
				asio_ns::ip::tcp::resolver resolver(io_context);
				for (auto &ep : resolver.resolve(address, std::to_string(port)))
				{
					endpoint = ep;
					break;
				}
			}

			asio_ns::ip::tcp::acceptor acceptor(io_context);
			acceptor.open(endpoint.protocol());
			acceptor.set_option(asio_ns::ip::tcp::acceptor::reuse_address(true));
			acceptor.bind(endpoint);
			acceptor.listen();

			acceptor.close();
		}
		catch (exception &e)
		{
			throw exception(std::string("Is server running already? ") + e.what());
		}

		int pid = fork();
		if (pid == -1)
			throw exception("Fork failed");

		if (pid == 0) // Child process
		{
			try
			{
				daemonize();

				open_log_file();

				std::clog << "starting server\n"
						  << "Listening to " << address << ':' << port << '\n';

				signal_catcher sc;
				sc.block();

				// Drop privileges
				if (not run_as_user.empty())
				{
					struct passwd *pw = getpwnam(run_as_user.c_str());
					if (pw == nullptr)
						throw exception(std::format("Failed to set uid to {}: {}", run_as_user, strerror(errno)));

					if (pw->pw_uid != getuid())
					{
						int ngroups = 0;
						if (getgrouplist(pw->pw_name, pw->pw_gid, nullptr, &ngroups) == -1 and ngroups > 0)
						{
							std::vector<gid_t> groups(ngroups);
							if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &ngroups) != -1 and
								setgroups(ngroups, groups.data()) == -1)
							{
								throw exception(std::format("Failed to set groups for {}: {}", run_as_user, strerror(errno)));
							}
						}

						if (setgid(pw->pw_gid) < 0)
							throw exception(std::format("Failed to set id for {}: {}", run_as_user, strerror(errno)));

						if (setuid(pw->pw_uid) < 0)
							throw exception(std::format("Failed to set uid for {}: {}", run_as_user, strerror(errno)));
					}
				}

				for (;;)
				{
					sc.block();

					std::unique_ptr<basic_server> server(m_factory());
					server->bind(address, port);

					std::thread t([nr_of_threads, &server]()
						{ server->run(nr_of_threads); });

					sc.unblock();
					int sig = sc.wait();

					std::clog << "Process " << getpid() << " received signal " << sig << "\n";

					server->stop();

					if (t.joinable())
						t.join();

					if (sig == SIGHUP)
					{
						// re-open log files
						open_log_file();

						std::clog << "re-starting server\n"
								  << "Listening to " << address << ':' << port << '\n';

						continue;
					}

					break;
				}
			}
			catch (const std::exception &ex)
			{
				std::println(std::clog, "Process terminated with exception: {}", ex.what());
			}

			if (fs::exists(m_pid_file, ec))
				fs::remove(m_pid_file, ec);

			if (ec)
				std::clog << "Removing pid file failed: " << ec.message() << '\n';

			// We're done. Exit
			_exit(0);
		}

		// avoid zombies
		int status, pid_c;
		pid_c = waitpid(-1, &status, WUNTRACED);

		if (pid_c != -1)
		{
			if (WIFSIGNALED(status) and WTERMSIG(status) != SIGKILL)
				std::clog << "child " << pid_c << " terminated by signal " << WTERMSIG(status) << '\n';
			// else
			// 	std::clog << "child terminated normally\n";
		}
	}

	return result;
}

int daemon::stop()
{
	int result = 1;

	if (pid_is_for_executable())
	{
		std::ifstream file(m_pid_file);
		if (not file.is_open())
			throw exception("Failed to open pid file");

		int pid;
		file >> pid;
		file.close();

		result = ::kill(pid, SIGINT);
		if (result != 0)
			std::clog << "Failed to stop process " << pid << ": " << strerror(errno) << '\n';

		// avoid zombies
		int status, pid_c;
		pid_c = waitpid(pid, &status, WUNTRACED);

		if (pid_c != -1)
		{
			if (WIFSIGNALED(status) and WTERMSIG(status) != SIGKILL)
				std::clog << "child " << pid_c << " terminated by signal " << WTERMSIG(status) << '\n';
			// else
			// 	std::clog << "child terminated normally\n";
		}

		std::error_code ec;
		if (fs::exists(m_pid_file, ec))
			fs::remove(m_pid_file, ec);
		if (ec)
			std::clog << "Could not remove pid file: " << ec.message() << '\n';
	}
	else
		throw exception("Not my pid file: " + m_pid_file);

	return result;
}

int daemon::status()
{
	int result;

	if (pid_is_for_executable())
	{
		std::clog << "server is running\n";
		result = 0;
	}
	else
	{
		std::clog << "server is not running\n";
		result = 1;
	}

	return result;
}

int daemon::reload()
{
	int result;

	if (pid_is_for_executable())
	{
		std::ifstream file(m_pid_file);
		if (not file.is_open())
			throw exception("Failed to open pid file");

		int pid;
		file >> pid;

		result = ::kill(pid, SIGHUP);
	}
	else
	{
		std::clog << "server is not running\n";
		result = 1;
	}

	return result;
}

int daemon::daemonize()
{
	using namespace std::literals;

	if (setsid() < 0)
		throw exception("Failed to create process group: "s + strerror(errno));

	// This in-between process should not catch SIGHUP
	(void)signal(SIGHUP, SIG_IGN);

	// fork again, to avoid being able to attach to a terminal device
	auto pid = fork();

	if (pid == -1)
		std::clog << "Fork failed\n";

	if (pid != 0)
		_exit(0);

	// write our pid to the pid file
	std::ofstream pidFile(m_pid_file);
	if (not pidFile.is_open())
		throw exception(std::format("Failed to write to {}: {}", m_pid_file, strerror(errno)));

	pidFile << getpid() << '\n';
	pidFile.close();

	if (chdir("/") != 0)
		throw exception("Cannot chdir to /: "s + strerror(errno));

	// close stdin
	close(STDIN_FILENO);
	(void)open("/dev/null", O_RDONLY); // NOLINT(hicpp-vararg)

	// The final process should however catch SIGHUP
	(void)signal(SIGHUP, SIG_DFL);

	return 0;
}

void daemon::open_log_file()
{
	// Flush the IO first
	std::cout.flush();
	std::cerr.flush();
	std::clog.flush();

	// open the log file
	int fd_out = open(m_stdout_log_file.c_str(), O_CREAT | O_APPEND | O_RDWR, 0644); // NOLINT(hicpp-vararg)
	int fd_err = (m_stderr_log_file == m_stdout_log_file)
	                 ? fd_out
	                 : open(m_stderr_log_file.c_str(), O_CREAT | O_APPEND | O_RDWR, 0644); // NOLINT(hicpp-vararg)

	if (fd_out < 0 or fd_err < 0)
	{
		if (fd_out >= 0)
			close(fd_out);
		if (fd_err >= 0 and fd_err != fd_out)
			close(fd_err);
		throw exception("Opening log file " + m_stdout_log_file);
	}

	// redirect stdout and stderr to the log file
	dup2(fd_out, STDOUT_FILENO);
	dup2(fd_err, STDERR_FILENO);

	// close the actual file descriptors to avoid leaks
	close(fd_out);
	if (fd_err != fd_out)
		close(fd_err);
}

bool daemon::pid_is_for_executable()
{
	using namespace std::literals;

	bool result = false;

	if (fs::exists(m_pid_file))
	{
		std::ifstream pidfile(m_pid_file);
		if (not pidfile.is_open())
			throw exception("Failed to open pid file " + m_pid_file + ": " + strerror(errno));

		int pid;
		pidfile >> pid;

		// if /proc/PID/exe points to our executable, this means we're already running
		char path[PATH_MAX] = "";
		if (readlink(("/proc/" + std::to_string(pid) + "/exe").c_str(), path, sizeof(path)) > 0)
		{
			char exe[PATH_MAX] = "";
			if (readlink("/proc/self/exe", exe, sizeof(exe)) == -1)
				throw exception("could not get exe path ("s + strerror(errno) + ")");

			result = strcmp(exe, path) == 0 or
			         (ends_with(path, " (deleted)") and starts_with(path, exe));
		}
		else if (errno == ENOENT)       // link file doesn't exist (can happen on e.g. macOS)
			result = kill(pid, 0) == 0; // simply test using kill with signal 0.
		else
			throw exception("Failed to read executable link : "s + strerror(errno));
	}

	return result;
}

#endif

} // namespace zeep::http
