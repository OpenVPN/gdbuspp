//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file signals/exceptions.hpp
 *
 *  @brief  Exceptions used within the DBus::Signals scope
 *
 */

#pragma once


#include "../exceptions.hpp"
#include "target.hpp"


namespace DBus {
namespace Signals {

class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &errm, GError *gliberr = nullptr);
    Exception(Target::Ptr target, const std::string &errm, GError *gliberr = nullptr);
};

} // namespace Signals
} // namespace DBus
