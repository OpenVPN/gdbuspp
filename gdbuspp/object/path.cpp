//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//


/**
 * @file object/path.cpp
 *
 * @brief Implementation of the DBus::Object::Path constructors and methods
 *
 */


#include <string>
#include <vector>

#include "exceptions.hpp"
#include "path.hpp"


namespace DBus::Object {

Path::Path(const char *str)
    : std::string(process_str(str))
{
}


Path::Path(const std::string &str)
    : Path(str.c_str())
{
}


Path::Path()
    : std::string()
{
}


const std::string Path::process_str(const char *str) const
{
    if (!str)
    {
        throw Object::Exception("Invalid D-Bus path; cannot be empty");
    }
    std::string ret(str);
    if (!ret.empty() && !g_variant_is_object_path(str))
    {
        throw Object::Exception("Invalid D-Bus path: " + ret);
    }
    return ret;
}


} // namespace DBus::Object
