//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once

/**
 * @file features/debug-log.hpp
 *
 * @brief  Internal debug logging features.  This requires the
 *         'internal_debug' option to be enabled via meson configure
 */

#include "build-config.h"

#ifdef GDBUSPP_INTERNAL_DEBUG

#include <string_view>
#include "../features/fmt-compat.hpp"
#include <fmt/format.h>

namespace GDBusPP::Debug {

void debug_log(std::string_view msg,
               std::string_view file,
               int line,
               std::string_view func);
}

#define GDBUSPP_LOG(fmt_str, ...) ::GDBusPP::Debug::debug_log(fmt::format(FMT_COMPILE(fmt_str), ##__VA_ARGS__), __FILE__, __LINE__, __func__)

#else

#define GDBUSPP_LOG(msg, ...)

#endif
