//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//


/**
 * @file features/debug-log.cpp
 *
 * @brief  Internal debug logging features.  This requires the
 *         'internal_debug' option to be enabled via meson configure
 */

#include "build-config.h"

#include "debug-log.hpp"

#include <unistd.h>
#include "../features/fmt-compat.hpp"
#include <fmt/format.h>
#include <fmt/printf.h>

namespace GDBusPP::Debug {
void debug_log(std::string_view msg,
               std::string_view file,
               int line,
               std::string_view func)
{
    fmt::print(stderr,
               FMT_COMPILE("[GDBus++ DEBUG (pid:{}) {{{}:{} {}}}] {}\n"),
               ::getpid(),
               file,
               line,
               func,
               msg);
}
} // namespace GDBusPP::Debug