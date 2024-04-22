//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C) 2023 -  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C) 2023 -  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/target.hpp
 *
 * @brief  Container for defining a D-Bus signal subscription or sending
 *         target.
 */

#include <string>

#include "../object/path.hpp"
#include "target.hpp"


namespace DBus::Signals {

Target::Ptr Target::Create(const std::string &busname,
                           const Object::Path &object_path,
                           const std::string &interface)
{
    return Target::Ptr(new Target(busname, object_path, interface));
}


Target::Target(const std::string &busname_,
               const Object::Path &object_path_,
               const std::string &interface)
    : busname(busname_), object_path(object_path_),
      object_interface(interface)
{
}


bool Target::operator==(const Target::Ptr cmp)
{
    return ((busname == cmp->busname)
            && (object_path == cmp->object_path)
            && (object_interface == cmp->object_interface));
}


bool Target::operator!=(const Target::Ptr &cmp)
{
    return !(this->operator==(cmp));
}

} // namespace DBus::Signals
