//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file   gen-constants.hpp
 *
 * @brief  Generates compile-time constant strings
 *
 *  This include file is weird by design.  It MUST be included AFTER
 *  a set of constant strings has been prepared.  These constants must
 *  be located in the Constants::Base namespace
 *
 *  Required constant std::string_view:
 *
 *   namespace Constants::Base {
 *       constexpr std::string_view BUSNAME{"..."};
 *       constexpr std::string_view ROOT_PATH{"..."};
 *       constexpr std::string_view INTERFACE{"..."};
 *   }
 *
 *  What the functions in this header provides is to generate constant
 *  strings for bus names, object paths and interfaces by prefixing them
 *  with the Constants::Base declared constant strings.
 *
 *  Constant strings created using the functions below cannot be based on
 *  a variable string input.  This ensures bus name, object paths and
 *  interfaces generated this way cannot be modified at runtime.
 *
 *  See tests/test-constants.hpp for an example how to use this header file.
 *
 */

#pragma once

#include <string>
#include <string_view>

/**
 *  Generates a service name, prefixed by the BUSNAME constant
 *  from the Constants::Base namespace
 *
 * @tparam sz              Size of the input string, deduced at compile time
 * @param  name            String containing the service name to append
 * @return constexpr auto  Returns a constant (string) with the BUSNAME
 *                         constant appended with the input service name.
 */
template <size_t sz>
constexpr auto GenServiceName(const char (&name)[sz])
{
    return Constants::Base::BUSNAME.data() + std::string(name, sz - 1);
}

/**
 *  Generates a constant object path, prefixed by the ROOT_PATH constant
 *  from the Constants::Base namespace
 *
 * @tparam sz              Size of the input string, deduced at compile time
 * @param  object_path     String containing the service name to append
 * @return constexpr auto  Returns a constant (string) with the ROOT_PATH
 *                         constant appended with the input object path.
 */
template <size_t sz>
constexpr auto GenPath(const char (&object_path)[sz])
{
    return Constants::Base::ROOT_PATH.data() + std::string(object_path, sz - 1);
}


/**
 *  Generates a constant object interface, prefixed by the INTERFACE
 *  constant from the Constants::Base namespace
 *
 * @tparam sz              Size of the input string, deduced at compile time
 * @param  name            String containing the service name to append
 * @return constexpr auto  Returns a constant (string) with the INTERFACE
 *                         constant appended with the input interface name.
 */
template <size_t sz>
constexpr auto GenInterface(const char (&interface)[sz])
{
    return Constants::Base::INTERFACE.data() + std::string(interface, sz - 1);
}
