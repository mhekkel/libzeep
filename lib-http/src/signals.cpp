// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <mutex>
#include <condition_variable>

#include "signals.hpp"

namespace zeep
{

// --------------------------------------------------------------------
//
//	Signal handling
//

#if _MSC_VER

struct signal_catcher_impl
{
	static BOOL __stdcall CtrlHandler(DWORD inCntrlType);

	static void signal(int inSignal);

	static int sSignal;
	static std::condition_variable sCondition;
	static std::mutex sMutex;
};

int signal_catcher_impl::sSignal;
std::condition_variable signal_catcher_impl::sCondition;
std::mutex signal_catcher_impl::sMutex;

BOOL signal_catcher_impl::CtrlHandler(DWORD inCntrlType)
{
	BOOL result = true;

	switch (inCntrlType)
	{
		// Handle the CTRL-C signal.
		case CTRL_C_EVENT:
			sSignal = SIGINT;
			break;

		// CTRL-CLOSE: confirm that the user wants to exit.
		case CTRL_CLOSE_EVENT:
			sSignal = SIGQUIT;
			break;

		// Pass other signals to the next handler.
		case CTRL_BREAK_EVENT:
			sSignal = SIGHUP;
			break;

		case CTRL_SHUTDOWN_EVENT:
		case CTRL_LOGOFF_EVENT:
			sSignal = SIGTERM;
			break;

		default:
			result = FALSE;
	}

	if (result)
		sCondition.notify_one();

	return result;
}

signal_catcher::signal_catcher()
	: mImpl(nullptr)
{
	if (not ::SetConsoleCtrlHandler(&signal_catcher_impl::CtrlHandler, true))
		throw std::runtime_error("Could not install control handler");
}

signal_catcher::~signal_catcher()
{
}

void signal_catcher::block()
{
}

void signal_catcher::unblock()
{
}

int signal_catcher::wait()
{
	boost::unique_lock<boost::mutex> lock(signal_catcher_impl::sMutex);
	signal_catcher_impl::sCondition.wait(lock);
	return signal_catcher_impl::sSignal;
}

void signal_catcher::signal_hangup(std::thread& t)
{
	signal_catcher_impl::CtrlHandler(CTRL_BREAK_EVENT);
}

#else

#include <unistd.h>
#include <signal.h>

// --------------------------------------------------------------------
//
//	signal
//

struct signal_catcher_impl
{
	sigset_t new_mask, old_mask;
};

signal_catcher::signal_catcher()
	: mImpl(new signal_catcher_impl)
{
	sigfillset(&mImpl->new_mask);
}

signal_catcher::~signal_catcher()
{
	delete mImpl;
}

void signal_catcher::block()
{
	pthread_sigmask(SIG_BLOCK, &mImpl->new_mask, &mImpl->old_mask);
}

void signal_catcher::unblock()
{
	pthread_sigmask(SIG_SETMASK, &mImpl->old_mask, nullptr);
}

int signal_catcher::wait()
{
	// Wait for signal indicating time to shut down.
	sigset_t wait_mask;
	sigemptyset(&wait_mask);
	sigaddset(&wait_mask, SIGINT);
	sigaddset(&wait_mask, SIGHUP);
	// sigaddset(&wait_mask, SIGCHLD);
	sigaddset(&wait_mask, SIGQUIT);
	sigaddset(&wait_mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &wait_mask, 0);

	int sig = 0;
	sigwait(&wait_mask, &sig);
	return sig;
}

void signal_catcher::signal_hangup(std::thread& t)
{
	// kill(getpid(), SIGHUP);
	pthread_kill(t.native_handle(), SIGHUP);
}

#endif

}