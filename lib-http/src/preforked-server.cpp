// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2023
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <iostream>
#include <cerrno>
#include <chrono>
#include <functional>

#if HTTP_SERVER_HAS_PREFORK

#include <sys/wait.h>

// for Hurd
#if not defined(WCONTINUED)
#define WCONTINUED 0
#endif

#include <zeep/http/connection.hpp>
#include <zeep/http/preforked-server.hpp>

namespace zeep::http
{

bool read_socket_from_parent(int fd_socket, asio_ns::ip::tcp::socket &socket)
{
	using native_handle_type = asio_ns::ip::tcp::socket::native_handle_type;

#if __APPLE__
	// macos is special...
	assert(CMSG_SPACE(sizeof(int)) == 16);
#endif

	struct msghdr msg;
	union
	{
		struct cmsghdr cm;
#if __APPLE__
		char control[16];
#else
		char control[CMSG_SPACE(sizeof(int))];
#endif
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_name = nullptr;
	msg.msg_namelen = 0;

	asio_ns::ip::tcp::socket::endpoint_type peer_endpoint;

	struct iovec iov[1];
	iov[0].iov_base = peer_endpoint.data();
	iov[0].iov_len = peer_endpoint.capacity();
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	bool result = false;
	int n = recvmsg(fd_socket, &msg, 0);
	if (n >= 0)
	{
		peer_endpoint.resize(n);

		struct cmsghdr *cmptr CMSG_FIRSTHDR(&msg);
		if (cmptr != nullptr and cmptr->cmsg_len == CMSG_LEN(sizeof(int)))
		{
			if (cmptr->cmsg_level != SOL_SOCKET)
				std::clog << "control level != SOL_SOCKET\n";
			else if (cmptr->cmsg_type != SCM_RIGHTS)
				std::clog << "control type != SCM_RIGHTS";
			else
			{
				/* Produces warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
				int fd = *(reinterpret_cast<native_handle_type*>(CMSG_DATA(cmptr)));
				*/
				native_handle_type *fdptr = reinterpret_cast<native_handle_type *>(CMSG_DATA(cmptr));
				int fd = *fdptr;
				if (fd >= 0)
				{
					socket.assign(peer_endpoint.protocol(), fd);
					result = true;
				}
			}
		}
	}

	return result;
}

class child_process
{
  public:
	child_process(std::function<basic_server *(void)> constructor, asio_ns::io_context &io_context, asio_ns::ip::tcp::acceptor &acceptor, int nr_of_threads)
		: m_constructor(constructor)
		, m_acceptor(acceptor)
		, m_socket(io_context)
		, m_nr_of_threads(nr_of_threads)
	{
		m_acceptor.async_accept(m_socket, std::bind(&child_process::handle_accept, this, std::placeholders::_1));
	}

	~child_process()
	{
		if (m_pid > 0) // should never happen
		{
			kill(m_pid, SIGKILL);

			const std::time_t now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

			int status;
			if (waitpid(m_pid, &status, WUNTRACED | WCONTINUED) != -1)
			{
				if (WIFSIGNALED(status) and WTERMSIG(status) != SIGKILL)
					std::clog << std::put_time(std::localtime(&now_t), "%F %T") << " child " << m_pid << " terminated by signal " << WTERMSIG(status) << '\n';
				// else
				// 	std::clog << std::put_time(std::localtime(&now_t), "%F %T") << " child terminated normally\n";
			}
			else
				std::clog << std::put_time(std::localtime(&now_t), "%F %T") << "error in waitpid: " << strerror(errno) << '\n';
		}
	}

	void stop();

  private:
	void handle_accept(const asio_system_ns::error_code &ec);

	void start();
	void run();

	std::function<basic_server *(void)> m_constructor;
	asio_ns::ip::tcp::acceptor &m_acceptor;
	asio_ns::ip::tcp::socket m_socket;
	int m_nr_of_threads;

	int m_pid = -1;
	int m_fd = -1;
};

void child_process::start()
{
	using namespace std::literals;

	// create a socket pair to pass the file descriptors through
	int sockfd[2];
	int err = socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
	if (err < 0)
		throw exception("Error creating socket pair: "s + strerror(errno));

	// fork
	m_pid = fork();
	if (m_pid < 0)
		throw exception("Error forking worker application: "s + strerror(errno));

	if (m_pid == 0) // child process
	{
		close(sockfd[0]);

		// remove the blocks on the signal handlers
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		pthread_sigmask(SIG_SETMASK, &wait_mask, 0);

		// Time to construct the Server object
		std::unique_ptr<basic_server> srvr(m_constructor());

		// run the server as a worker
		std::thread t(std::bind(&basic_server::run, srvr.get(), m_nr_of_threads));

		// now start the processing loop passing on file descriptors read
		// from the parent process
		try
		{
			for (;;)
			{
				std::shared_ptr<connection> conn(new connection(srvr->get_io_context(), *srvr));

				if (not read_socket_from_parent(sockfd[1], conn->get_socket()))
					break;

				conn->start();
			}
		}
		catch (std::exception &e)
		{
			std::clog << "Exception caught: " << e.what() << '\n';
			exit(1);
		}

		srvr->stop();
		t.join();

		exit(0);
	}

	// close one end of the pipe, save the other
	m_fd = sockfd[0];
	close(sockfd[1]);
}

void child_process::stop()
{
	if (m_fd > 0)
	{
		// close the socket to the worker, this should terminate the child if it is still alive
		close(m_fd);
		m_fd = -1;
	}

	if (m_pid != -1)
	{
		// however, sometimes it doesn't, so we have to take some serious action
		// Anyway, we'll wait until child dies to avoid zombies
		// and to make sure the client really stops...

		int count = 5; // wait five seconds before killing client
		int status;

		while (count-- > 0)
		{
			if (waitpid(m_pid, &status, WUNTRACED | WCONTINUED | WNOHANG) == -1)
				break;

			if (WIFEXITED(status))
				break;

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		if (not WIFEXITED(status))
			kill(m_pid, SIGKILL);

		m_pid = -1;
	}
}

void child_process::handle_accept(const asio_system_ns::error_code &ec)
{
	if (ec)
	{
		std::clog << "Accept failed: " << ec << '\n';
		return;
	}

	try
	{
		if (m_pid == -1 or m_fd == -1)
			start();

		using native_handle_type = asio_ns::ip::tcp::socket::native_handle_type;

		struct msghdr msg;
		union
		{
			struct cmsghdr cm;
#if __APPLE__
			char control[16];
#else
			char control[CMSG_SPACE(sizeof(native_handle_type))];
#endif
		} control_un;

		msg.msg_control = control_un.control;
		msg.msg_controllen = sizeof(control_un.control);

		struct cmsghdr *cmptr = CMSG_FIRSTHDR(&msg);
		cmptr->cmsg_len = CMSG_LEN(sizeof(int));
		cmptr->cmsg_level = SOL_SOCKET;
		cmptr->cmsg_type = SCM_RIGHTS;
		/* Procudes warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
		 *(reinterpret_cast<native_handle_type*>(CMSG_DATA(cmptr))) = socket.native();
		 */
		native_handle_type *fdptr = reinterpret_cast<native_handle_type *>(CMSG_DATA(cmptr));
		*fdptr = m_socket.native_handle();

		msg.msg_name = nullptr;
		msg.msg_namelen = 0;

		struct iovec iov[1];
		iov[0].iov_base = m_socket.remote_endpoint().data();
		iov[0].iov_len = m_socket.remote_endpoint().size();
		msg.msg_iov = iov;
		msg.msg_iovlen = 1;

		int err = sendmsg(m_fd, &msg, 0);
		if (err < 0)
		{
			std::clog << "Error passing file descriptor: " << strerror(errno) << '\n';
			// ec = std::make_error_code(errno);
			stop();
		}
	}
	catch (const std::exception &e)
	{
		std::clog << "error writing socket to client: " << e.what() << '\n';

		reply r = reply::stock_reply(service_unavailable);

		auto buffers = r.to_buffers();

		try
		{
			asio_ns::write(m_socket, buffers);
		}
		catch (const std::exception &e)
		{
			std::clog << e.what() << '\n';
		}
	}

	m_socket.close();

	m_acceptor.async_accept(m_socket, std::bind(&child_process::handle_accept, this, std::placeholders::_1));
}

// --------------------------------------------------------------------

preforked_server::preforked_server(std::function<basic_server *(void)> constructor)
	: m_constructor(constructor)
{
	m_lock.lock();
}

preforked_server::~preforked_server()
{
}

void preforked_server::run(const std::string &address, short port, int nr_of_processes, int nr_of_threads)
{
	// first wait until we are allowed to start listening
	std::unique_lock<std::mutex> lock(m_lock);

	asio_ns::ip::tcp::acceptor acceptor(m_io_context);

	// then bind the address here
	asio_ns::ip::tcp::endpoint endpoint;
	try
	{
		endpoint = asio_ns::ip::tcp::endpoint(asio_ns::ip::make_address(address), port);
	}
	catch (const std::exception &e)
	{
		asio_ns::ip::tcp::resolver resolver(m_io_context);
		asio_ns::ip::tcp::resolver::query query(address, std::to_string(port));
		endpoint = *resolver.resolve(query);
	}

	acceptor.open(endpoint.protocol());
	acceptor.set_option(asio_ns::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();

	std::vector<std::thread> threads;

	for (int i = 0; i < nr_of_processes; ++i)
	{
		threads.emplace_back([this, nr_of_threads, &acceptor]()
			{
			auto work = asio_ns::make_work_guard(m_io_context);

			child_process p(m_constructor, m_io_context, acceptor, nr_of_threads);
			
			m_io_context.run(); });
	}

	for (auto &t : threads)
		t.join();
}

void preforked_server::start()
{
	m_lock.unlock();
}

void preforked_server::stop()
{
	m_io_context.stop();
}

} // namespace zeep::http

#endif
