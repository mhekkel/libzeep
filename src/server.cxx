// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#if USE_DATE_H
# include <date/date.h>
# include <date/tz.h>
#endif

#include <chrono>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <list> // for list
#include <memory>
#include <new>
#include <set> // for set
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple> // for tie
#include <type_traits>
#include <vector>

#include "zeep/http/asio.hpp"

module zeep;

#include "server.cpp"
