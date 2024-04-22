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
#include "../proxy/utils.hpp"
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


const char *Target::GetBusName(std::shared_ptr<Proxy::Utils::DBusServiceQuery> service_qry)
{
    // Only do a unique busname lookup if:
    //  the busname does not contain a unique bus name (starts with ':1.')
    //  && a service_qry object is available
    //  && the well-known busname is not empty (expect matching on a bus name)
    //  && it has not already looked it up (unique_busname is not empty)
    if (busname.find(":1.") == std::string::npos
        && service_qry
        && !busname.empty()
        && unique_busname.empty())
    {
        unique_busname = service_qry->GetNameOwner(busname);
    }

    // Only return the unique busname if we have it.  Otherwise use the
    // well-known busname if set.  If both are unset, nullptr is returned
    // as that is accepted by the glib2 APIs to not match on bus names
    return (!unique_busname.empty() ? unique_busname.c_str()
                                    : (!busname.empty() ? busname.c_str() : nullptr));
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
