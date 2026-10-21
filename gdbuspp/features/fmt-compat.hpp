//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C) 2026-  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C) 2026-  Karol Adrianek <karol.adrianek@openvpn.com>
//

#pragma once

/**
 * @file fmt-compat.hpp
 *
 * @brief  Compatibility layer for older fmtlib releases.  Include this
 *         header instead of <fmt/compile.h> to make FMT_COMPILE() and
 *         fmt::ostream_formatter available on all supported fmtlib versions.
 */

#include <fmt/format.h>

//  FMT_COMPILE() exists since fmtlib v7.0, but fmt::print() only accepts
//  compiled format strings since v8.0.  Fall back to FMT_STRING(), which
//  still checks the format string at compile time.
#if FMT_VERSION >= 80000
#include <fmt/compile.h>
#else
#define FMT_COMPILE(s) FMT_STRING(s)
#endif // FMT_VERSION >= 80000

#if FMT_VERSION < 90000
#include <sstream>

namespace fmt {

/**
 *  fmt::ostream_formatter was introduced in fmtlib v9.0.  This provides
 *  the same behaviour for older releases, formatting a value via its
 *  std::ostream operator<<().
 */
struct ostream_formatter : formatter<string_view>
{
    template <typename T, typename FormatContext>
    auto format(const T &value, FormatContext &ctx) -> decltype(ctx.out())
    {
        std::ostringstream os;
        os << value;
        return formatter<string_view>::format(os.str(), ctx);
    }
};

} // namespace fmt
#endif // FMT_VERSION < 90000
