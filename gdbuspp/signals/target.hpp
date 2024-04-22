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
 *
 */

#pragma once

#include <memory>
#include <sstream>
#include <vector>
#include <glib.h>

#include "../object/path.hpp"

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

    const std::string busname;
    const Object::Path object_path;
    const std::string object_interface;

    /**
     *  Create a new target object
     *
     * @param busname       std::string with the D-Bus bus name (unique or
     *                      well-known)
     * @param object_path   DBus::Object::Path with the D-Bus object path
     * @param interface     std::string with the D-Bus object interface scope
     *
     * @return Target::Ptr
     */
    [[nodiscard]] static Target::Ptr Create(const std::string &busname,
                                            const Object::Path &object_path,
                                            const std::string &interface);


    bool operator==(const Target::Ptr cmp);
    bool operator!=(const Target::Ptr &cmp);

    friend std::ostream &operator<<(std::ostream &os, const Target::Ptr &tgt)
    {
        return os << "Signals::Target("
                  << "busname=" << tgt->busname << ", "
                  << "object_path=" << tgt->object_path << ", "
                  << "interface=" << tgt->object_interface << ")";
    }


  private:
    Target(const std::string &busname_,
           const Object::Path &object_path_,
           const std::string &interface);
};

} // namespace Signals
} // namespace DBus
