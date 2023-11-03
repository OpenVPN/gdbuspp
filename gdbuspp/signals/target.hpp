//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file signals/target.hpp
 *
 * @brief  Container for defining a D-Bus signal subscription or sending
 *         target.
 *
 */

#pragma once

#include <memory>
#include <sstream>
#include <vector>
#include <glib.h>


namespace DBus {
namespace Signals {


/**
 *  Defines a target "address" for sending or receiving a D-Bus signal.
 */
class Target
{
  public:
    using Ptr = std::shared_ptr<Signals::Target>;

    /**
     *  Simple array (std::vector) with a collection of target objects.
     *  This is used by Signals::Emit to send the same signal to more
     *  recipients
     */
    using Collection = std::vector<Target::Ptr>;

    /**
     *  Create a new target object
     *
     * @param busname       std::string with the D-Bus bus name (unique or
     *                      well-known)
     * @param object_path   std::string with the D-Bus object path
     * @param interface     std::string with the D-Bus object interface scope
     *
     * @return Target::Ptr
     */
    [[nodiscard]] static Target::Ptr Create(const std::string &busname,
                                            const std::string &object_path,
                                            const std::string &interface)
    {
        return Target::Ptr(new Target(busname, object_path, interface));
    }

    const std::string busname;
    const std::string object_path;
    const std::string object_interface;


    bool operator==(const Target::Ptr cmp)
    {
        return ((busname == cmp->busname)
                && (object_path == cmp->object_path)
                && (object_interface == cmp->object_interface));
    }

    bool operator!=(const Target::Ptr &cmp)
    {
        return !(this->operator==(cmp));
    }

  private:
    Target(const std::string &busname_,
           const std::string &object_path_,
           const std::string &interface)
        : busname(busname_), object_path(object_path_),
          object_interface(interface)
    {
    }
};

} // namespace Signals
} // namespace DBus
