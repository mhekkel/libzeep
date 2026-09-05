// Copyright Maarten L. Hekkelman 2026
//
// SPDX-License-Identifier: BSD-2-Clause

module;

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#if __has_include(<flat_map>)
# include <flat_map>
#endif
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#if __has_include(<nlohmann/json.hpp>)
# include <nlohmann/json.hpp>
# define HAVE_NLOHMANN_JSON 1
#endif
#include <optional>
#include <ranges>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <version>

#include "zeep/http/asio.hpp"

export module zeep;

// export import :core;
// export import :el;
// export import :http;

import zeem;

#define ZEEP_EXPORT export
#define ZEEP_INLINE

#if defined(_WIN32) && defined(ZEEP_SHARED_BUILD)
# define ZEEP_API __declspec(dllexport)
#else
# define ZEEP_API
#endif

#include "zeep/config.hpp"

// clang-format off

#include "zeep/crypto.hpp"
#include "zeep/exception.hpp"
#include "zeep/streambuf.hpp"
#include "zeep/unicode-support.hpp"
#include "zeep/uri.hpp"

// clang-format on

// clang-format off

#include "zeep/el/object.hpp"
#include "zeep/el/processing.hpp"
#include "zeep/el/serializer.hpp"

// clang-format on

// clang-format off

#include "zeep/http/status.hpp"
#include "zeep/http/header.hpp"
#include "zeep/http/request.hpp"
#include "zeep/http/reply.hpp"
#include "zeep/http/client.hpp"
#include "zeep/http/message-parser.hpp"
#include "zeep/http/access-control.hpp"
#include "zeep/http/connection.hpp"
#include "zeep/http/tag-processor.hpp"
#include "zeep/http/template-processor.hpp"
#include "zeep/http/server.hpp"
#include "zeep/http/scope.hpp"
#include "zeep/http/controller.hpp"
#include "zeep/http/daemon.hpp"
#include "zeep/http/error-handler.hpp"
#include "zeep/http/html-controller.hpp"
#include "zeep/http/login-controller.hpp"
#include "zeep/http/security.hpp"
#include "zeep/http/soap-controller.hpp"

// clang-format on
