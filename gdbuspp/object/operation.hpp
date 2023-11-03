// GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/operation.hpp
 *
 * @brief Declaration of DBus::Object::Operation
 */

#pragma once

#include <string>

namespace DBus {
namespace Object {


/**
 *  Defined D-Bus operations the DBus::Object is expected to
 *  receive.
 *
 *  This is used by the Authz::Request and AsyncProcess::Request classes
 */
enum class Operation
{
    NONE,         ///< No operation prepared
    METHOD_CALL,  ///< Call a method in the D-Bus object
    PROPERTY_GET, ///< Read the property value in the D-Bus object
    PROPERTY_SET  ///< Change the property vlaue in the D-Bus object
};


/**
 * Helper function converting DBus::Object::Operation value to
 * a readable string

 * @param op                   Object::Operation
 * @return const std::string   std::string of Object::Operation
 */
const std::string OperationString(const Operation &op);


} // namespace Object
} // namespace DBus
