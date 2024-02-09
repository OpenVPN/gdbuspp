//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file gen-constants.hpp
 *
 * @brief This is a fragment header, to be included after a Constants::Base
 *        namespace has been declared with a few std::string_view "constants"
 *        declaring the base prefix for D-Bus service names, object paths and
 *        interface names.
 *
 *        The purpose of this header is to create compile-time constant strings
 *        while being a bit more flexible when needing a slight variation at
 *        the implementation point.  This is an alternative to using #define
 *        macros, where this approach will ensure the end result is always
 *        of a std::string data type.
 *
 *        An example:
 *
 *        @code
 *        namespace Constants {
 *        namespace Base {
 *
 *        constexpr std::string_view BUSNAME{"net.openvpn.example."};
 *        constexpr std::string_view ROOT_PATH{"/gdbuspp/example/"};
 *        constexpr std::string_view INTERFACE{"gdbuspp.example."};
 *
 *        }
 *        #include <gdbuspp/gen-constants.hpp>
 *        }
 *
 *        //
 *        // Generating a service name, an object path and an interface
 *        //
 *
 *        std::string service_name = Constants::GenServiceName("service");
 *        // result:  "net.openvpn.example.service";
 *
 *        std::string object_path = Constants::GenPath("object_path");
 *        // result:  "/gdbuspp/example/object_path";
 *
 *        std::string service_name = Constants::GenInterface("interface");
 *        // result:  "gdbuspp.example.interface";
 *
 *        @endcode
 *
 *        See also tests/test-constants.hpp for another example
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
 *  This function will return std::string, and not DBus::Object::Path, since
 *  the object_path provided to this function may not be the final and complete
 *  path.  And that would make the path validation check in DBus::Object::Path
 *  throw an exception.
 *
 * @tparam sz              Size of the input string, deduced at compile time
 * @param  object_path     String containing the service name to append
 * @return constexpr auto  Returns a constant (std::string) with the ROOT_PATH
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
