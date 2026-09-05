// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

// module;

#include <condition_variable>
#include <stdexcept>
#include <mutex>

#if _WIN32
# include <Windows.h>
# include <signal.h>
# include <wincon.h>
#else
# include <csignal>
# include <pthread.h>
# include <unistd.h>
#endif

#include "detail/signals.hpp"

#include "signals.cpp"
