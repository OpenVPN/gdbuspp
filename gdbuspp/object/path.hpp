//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//


/**
 * @file object/path.hpp
 *
 * @brief Declaration of a std::string variant used for D-Bus object paths
 *        In C++, object paths are essentially ordinary strings with more
 *        restrictions on valid characters and content.
 *
 *        More details on the D-Bus object path data type:
 *        https://docs.gtk.org/glib/type_func.Variant.is_object_path.html
 *
 */

#pragma once

#include <string>
#include <vector>

#include "exceptions.hpp"

namespace DBus::Object {

/**
 *  The DBus::Object::Path extends std::string in a way which makes
 *  it a distinguishable and separate data type in C++, while having
 *  the same possibilities of interacting with it as if it was an
 *  ordinary string.
 *
 *  This will make it easier when converting data from and to GVariant
 *  containers.  The C++ templates in glib2/utils.hpp will derive the
 *  proper D-Bus data type from DBus::Object::Path and vice versa.
 */
class Path : public std::string
{
  public:
    using List = std::vector<Path>; ///< Helper type for list of paths

    Path(const char *str);
    Path(const std::string &str);
    Path();

  private:
    /**
     *  Internal helper to validate the path string.
     *
     *  This method will call the g_variant_is_object_path() function
     *  in glib2 to validate the path.
     *
     * @param str    char* based C string containing the D-Bus path to evaluate
     *
     * @return const std::string of a valid D-Bus path to be stored in the object
     * @throws DBus::Object::Exception() on invalid D-Bus object path strings
     */
    const std::string process_str(const char *str) const;
};

} // namespace DBus::Object
