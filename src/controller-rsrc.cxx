// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cassert>
#include <chrono>
#include <climits>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <system_error>
#include <utility>

#if _WIN32 and not defined(WIN32_LEAN_AND_MEAN)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#if __has_include(<unistd.h>)
# include <unistd.h>
#endif

module zeep;

#include "controller-rsrc.cpp"
