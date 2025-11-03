//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   test-constants.hpp
 *
 * @brief  Collection of shared constant values used by the
 *         various test programs
 *
 */

#pragma once

#include <string>
#include <string_view>
#include <cstring>

namespace Tests::Constants {

/**
 *  Constant values of prefixes used by the test programs
 */
namespace Base {
constexpr std::string_view BUSNAME{"net.openvpn.gdbuspp.test."};
constexpr std::string_view ROOT_PATH{"/gdbuspp/tests/"};
constexpr std::string_view INTERFACE{"gdbuspp.test."};
} // namespace Base

// Include the constant generator logic
#include "../gdbuspp/gen-constants.hpp"

} // namespace Tests::Constants
