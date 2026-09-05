// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <mutex>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <thread>
#include <utility>

#if __has_include(<sys/mman.h>)
# include <sys/mman.h>
#endif

module zeep;

#include "detail/glob.hpp"

#include "security.cpp"
