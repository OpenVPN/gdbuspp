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

#pragma once

#include "../exceptions.hpp"

namespace DBus {
namespace Credentials {

class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &catg, const std::string &message, GError *gliberr = nullptr);
};

} // namespace Credentials
} // namespace DBus
