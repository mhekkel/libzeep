// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <utility>

#ifndef _WIN32
# include <grp.h>
# include <limits.h>
# include <pwd.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

#include "detail/signals.hpp"
#include "zeep/http/asio.hpp"

module zeep;

#include "zeep/config.hpp"

#include "daemon.cpp"
