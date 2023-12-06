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

#include <iostream>

#define GDBUSPP_LOG(msg) std::cout << "[GDBus++ DEBUG "                                              \
                                   << "{" << __FILE__ << ":" << __LINE__ << " " << __func__ << "}] " \
                                   << msg << std::endl;

#else

#define GDBUSPP_LOG(msg)

#endif
