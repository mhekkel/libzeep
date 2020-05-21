// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

// Source code specifically for Unix/Linux
// Utilitie routines to build daemon processes

#include <pwd.h>
#include <wait.h>

#include <mutex>
#include <fstream>
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include <zeep/http/daemon.hpp>
#include <zeep/http/preforked-server.hpp>

namespace ba = boost::algorithm;
namespace fs = std::filesystem;

using namespace std::literals;

namespace zeep::http
{

daemon::daemon(server_factory_type&& factory, const std::string& pid_file,
	const std::string& stdout_log_file, const std::string& stderr_log_file)
	: m_factory(std::move(factory)), m_pid_file(pid_file)
	, m_stdout_log_file(stdout_log_file), m_stderr_log_file(stderr_log_file)
{
}

daemon::daemon(server_factory_type&& factory, const char* name)
	: daemon(std::forward<server_factory_type>(factory), "/var/run/"s + name,
		"/var/log/"s + name + "/access.log", "/var/log/"s + name + "/error.log")
{
}

int daemon::start(const std::string& address, uint16_t port, size_t nr_of_threads, const std::string& run_as_user)
{
	int result = 0;
	
	if (pid_is_for_executable())
	{
		std::cerr << "Server is already running." << std::endl;
		result = 1;
	}
	else
	{
        try
        {
            boost::asio::io_service io_service;
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(address, std::to_string(port));
            boost::asio::ip::tcp::endpoint endpoint(*resolver.resolve(query));

            boost::asio::ip::tcp::acceptor acceptor(io_service);
            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();
            
            acceptor.close();
        }
        catch (exception& e)
        {
            throw std::runtime_error(std::string("Is server running already? ") + e.what());
        }
        
       	daemonize();
        
        run_main_loop(address, port, nr_of_threads, run_as_user);
        
        if (fs::exists(m_pid_file))
            try { fs::remove(m_pid_file); } catch (...) {}
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
            throw std::runtime_error("Failed to open pid file");

        int pid;
        file >> pid;
        file.close();

	    result = ::kill(pid, SIGINT);
	    if (result != 0)
	        std::cerr << "Failed to stop process " << pid << ": " << strerror(errno) << std::endl;
        try
        {
            if (fs::exists(m_pid_file))
                fs::remove(m_pid_file);
        }
        catch (...) {}

    }
    else
        throw std::runtime_error("Not my pid file: " + m_pid_file);

    return result;
}

int daemon::status()
{
    int result;

    if (pid_is_for_executable())
    {
		std::cerr << "server is running" << std::endl;
        result = 0;
    }
    else
    {
		std::cerr << "server is not running" << std::endl;
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
            throw std::runtime_error("Failed to open pid file");

        int pid;
        file >> pid;

        result = ::kill(pid, SIGHUP);
    }
    else
    {
		std::cerr << "server is not running" << std::endl;
        result = 1;
    }

    return result;
}

int daemon::run_foreground(const std::string& address, uint16_t port)
{
	int result = 0;
	
	if (pid_is_for_executable())
	{
		std::cerr << "Server is already running." << std::endl;
		result = 1;
	}
	else
	{
        try
        {
            boost::asio::io_service io_service;
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(address, std::to_string(port));
            boost::asio::ip::tcp::endpoint endpoint(*resolver.resolve(query));

            boost::asio::ip::tcp::acceptor acceptor(io_service);
            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();
            
            acceptor.close();
        }
        catch (exception& e)
        {
            throw std::runtime_error(std::string("Is server running already? ") + e.what());
        }
        
		sigset_t new_mask, old_mask;
		sigfillset(&new_mask);
		pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

		std::unique_ptr<server> s(m_factory());
		s->bind(address, port);
		std::thread t(std::bind(&server::run, s.get(), 1));

		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGHUP);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		sigaddset(&wait_mask, SIGCHLD);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);

		int sig;
		sigwait(&wait_mask, &sig);

		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		s->stop();

		if (t.joinable())
			t.join();
	}
	
	return result;
}

void daemon::daemonize()
{
    int pid = fork();

    if (pid == -1)
    {
        std::cerr << "Fork failed" << std::endl;
        exit(1);
    }

    // exit the parent (=calling) process
    if (pid != 0)
        _exit(0);

    if (setsid() < 0)
    {
        std::cerr << "Failed to create process group: " << strerror(errno) << std::endl;
        exit(1);
    }

    // it is dubious if this is needed:
    signal(SIGHUP, SIG_IGN);

    // fork again, to avoid being able to attach to a terminal device
    pid = fork();

    if (pid == -1)
        std::cerr << "Fork failed" << std::endl;

    if (pid != 0)
        _exit(0);

    // write our pid to the pid file
    std::ofstream pidFile(m_pid_file);
    if (not pidFile.is_open())
    {
        std::cerr << "Failed to write to " << m_pid_file << ": " << strerror(errno) << std::endl;
        exit(1);
    }

    pidFile << getpid() << std::endl;
    pidFile.close();

    if (chdir("/") != 0)
    {
        std::cerr << "Cannot chdir to /: " << strerror(errno) << std::endl;
        exit(1);
    }

    // close stdin
    close(STDIN_FILENO);
    open("/dev/null", O_RDONLY);
}

void daemon::open_log_file()
{
	// open the log file
	int fd_out = open(m_stdout_log_file.c_str(), O_CREAT|O_APPEND|O_RDWR, 0644);
	if (fd_out < 0)
	{
		std::cerr << "Opening log file " << m_stdout_log_file << " failed" << std::endl;
		exit(1);
	}

	int fd_err;

	if (m_stderr_log_file == m_stdout_log_file)
		fd_err = fd_out;
	else
	{
		fd_err = open(m_stderr_log_file.c_str(), O_CREAT|O_APPEND|O_RDWR, 0644);
		if (fd_err < 0)
		{
			std::cerr << "Opening log file " << m_stderr_log_file << " failed" << std::endl;
			exit(1);
		}
	}

	// redirect stdout and stderr to the log file
	dup2(fd_out, STDOUT_FILENO);
	dup2(fd_err, STDERR_FILENO);

	// close the actual file descriptors to avoid leaks
	close(fd_out);
	if (fd_err != fd_out)
		close(fd_err);
}

bool daemon::run_main_loop(const std::string& address, uint16_t port, size_t nr_of_threads, const std::string& run_as_user)
{
	int sig = 0;

	int restarts = 0;

	for (;;)
	{
		auto start = time(nullptr);

		open_log_file();

		if (sig == 0)
			std::cerr << "starting server" << std::endl;
		else
			std::cerr << "restarting server" << std::endl;

		std::cerr << "Listening to " << address << ':' << port << std::endl;

		sigset_t new_mask, old_mask;
		sigfillset(&new_mask);
		pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

		preforked_server server([=]()
		{
			try
			{
				if (not run_as_user.empty())
				{
					struct passwd* pw = getpwnam(run_as_user.c_str());
					if (pw == NULL or setuid(pw->pw_uid) < 0)
					{
						std::cerr << "Failed to set uid to " << run_as_user << ": " << strerror(errno) << std::endl;
						exit(1);
					}
				}
				
				return m_factory();
			}
			catch (const exception& e)
			{
				std::cerr << "Failed to launch server: " << e.what() << std::endl;
				exit(1);
			}
		});
		
		std::thread t(std::bind(&zeep::http::preforked_server::run, &server, address, port, nr_of_threads));
		
		try
		{
			server.start();
		}
		catch (const exception& ex)
		{
			std::cerr << std::endl
				<< "Exception running server: " << std::endl
				<< ex.what() << std::endl
				<< std::endl;
			exit(1);
		}

		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGHUP);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		sigaddset(&wait_mask, SIGCHLD);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);

		sigwait(&wait_mask, &sig);

		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		server.stop();
		t.join();

		// did the client crash within the time window?
		if (time(nullptr) - start < m_restart_time_window)
		{
			if (++restarts >= m_max_restarts)
			{
				std::cerr << "aborting due to excessive restarts" << std::endl;
				break;
			}
		}
		else	// reset counter
			restarts = 0;

		if (sig == SIGCHLD)
		{
			int status, pid;
			pid = waitpid(-1, &status, WUNTRACED);

			if (pid != -1 and WIFSIGNALED(status))
				std::cerr << "child " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
			continue;
		}

		break;
	}

	return sig == SIGHUP;
}

bool daemon::pid_is_for_executable()
{
	using namespace std::literals;

	bool result = false;

	if (fs::exists(m_pid_file))
	{
		std::ifstream pidfile(m_pid_file);
		if (not pidfile.is_open())
			throw std::runtime_error("Failed to open pid file " + m_pid_file + ": " + strerror(errno));

		int pid;
		pidfile >> pid;

		// if /proc/PID/exe points to our executable, this means we're already running
		char path[PATH_MAX] = "";
		if (readlink(("/proc/" + std::to_string(pid) + "/exe").c_str(), path, sizeof(path)) > 0)
		{
			char exe[PATH_MAX] = "";
			if (readlink("/proc/self/exe", exe, sizeof(exe)) == -1)
				throw std::runtime_error("could not get exe path ("s + strerror(errno) + ")");

			result = strcmp(exe, path) == 0 or
					 (ba::ends_with(path, " (deleted)") and ba::starts_with(path, exe));
		}
		else if (errno == ENOENT) // link file doesn't exist
			result = false;
		else
			throw std::runtime_error("Failed to read executable link : "s + strerror(errno));
	}

	return result;
}

}