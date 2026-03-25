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

#include <glib.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

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
namespace {

/**
 *  Composes a string containing the details of a Signals::Target
 *
 * @param target               Signals::Target object to retrieve info from
 * @return const std::string
 */
static std::string compose_errclass(Target::Ptr target)
{
    std::vector<std::string> details;
    if (!target->busname.empty())
    {
        details.push_back(fmt::format(FMT_COMPILE("busname={}"), target->busname));
    }
    if (!target->object_path.empty())
    {
        details.push_back(fmt::format(FMT_COMPILE("object_path={:?}"), std::string(target->object_path)));
    }
    if (!target->object_interface.empty())
    {
        details.push_back(fmt::format(FMT_COMPILE("interface={:?}"), target->object_interface));
    }
    return fmt::format(FMT_COMPILE("DBus::Signal::Target({})"), fmt::join(details, ", "));
}

} // namespace


Exception::Exception(const std::string &errm, GError *gliberr)
    : DBus::Exception("DBus::Signals", errm, gliberr)
{
}

Exception::Exception(Target::Ptr target, const std::string &errm, GError *gliberr)
    : DBus::Exception(compose_errclass(target), errm, gliberr)
{
}

} // namespace Signals
} // namespace DBus
