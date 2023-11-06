//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file exceptions.hpp
 *
 * @brief Declaration of DBus::Credentials::Exception
 */


#include "exceptions.hpp"


namespace DBus {
namespace Credentials {

Credentials::Exception::Exception(const std::string &catg,
                                  const std::string &message,
                                  GError *gliberr)
    : DBus::Exception("DBus::Credentials::" + catg, message, gliberr)
{
}

} // namespace Credentials
} // namespace DBus
