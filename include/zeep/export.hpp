// SPDX-FileCopyrightText: Maarten L. Hekkelman, 2026
// SPDX-License-Identifier: BSL-1.0

#pragma once

#ifndef ZEEP_EXPORT
# define ZEEP_EXPORT
#endif

#ifndef ZEEP_INLINE
# define ZEEP_INLINE inline
#endif

#ifndef ZEEP_API
# if defined(_WIN32) && defined(ZEEP_SHARED_BUILD)
#  define ZEEP_API __declspec(dllexport)
# else
#  define ZEEP_API
# endif
#endif
