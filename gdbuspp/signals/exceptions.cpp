//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file signals/exceptions.cpp
 *
 *  @brief  Implementation of the DBus::Signals::Exceptions class
 *
 */

#include <sstream>
#include <glib.h>

#include "../exceptions.hpp"
#include "target.hpp"
#include "exceptions.hpp"


namespace DBus {
namespace Signals {

/**
 *  This namespace is only used for holding library internal data.
 *
 *  The intention is to avoid hiding information useful while debugging,
 *  but at the same time keep it in a separate namespace indicating it
 *  is not to be directly exposed to the users of the library.
 */
namespace _private {

/**
 *  Composes a string containing the details of a Signals::Target
 *
 * @param target               Signals::Target object to retrieve info from
 * @return const std::string
 */
const std::string compose_errclass(Target::Ptr target)
{
    std::ostringstream erc;
    erc << "DBus::Signal::Target(";
    if (!target->busname.empty())
    {
        erc << "busname='" << target->busname << "', ";
    }
    if (!target->object_path.empty())
    {
        erc << "object_path='" << target->object_path << "', ";
    }
    if (!target->object_interface.empty())
    {
        erc << "interface='" << target->object_interface << "'";
    }
    erc << ")";
    return erc.str();
}
} // namespace _private


Exception::Exception(const std::string &errm, GError *gliberr)
    : DBus::Exception("DBus::Signals", errm, gliberr)
{
}

Exception::Exception(Target::Ptr target, const std::string &errm, GError *gliberr)
    : DBus::Exception(_private::compose_errclass(target), errm, gliberr)
{
}

} // namespace Signals
} // namespace DBus
