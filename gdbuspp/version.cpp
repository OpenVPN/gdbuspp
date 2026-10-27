//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C) 2026-  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C) 2026-  David Sommerseth <davids@openvpn.net>
//

/**
 * @file version.cpp
 *
 * @brief  Provides an API for retrieving version information
 *         the currently active GDBus++ version and API
 */


#include <string>
#include "features/fmt-compat.hpp"
#include <fmt/format.h>

#include "build-version.hpp"

namespace GDBusPP {

std::string GetVersion()
{
    return fmt::format(FMT_COMPILE("GDBus++ {} (API version {})"), GDBUSPP_VERSION, GDBUSPP_API_VERSION);
}

uint16_t GetAPIversion()
{
    return GDBUSPP_API_VERSION;
}

} // namespace GDBusPP