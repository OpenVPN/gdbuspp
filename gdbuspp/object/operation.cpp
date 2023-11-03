// GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/operation.cpp
 *
 * @brief Implementation of DBus::Object::Operation helper functions
 */

#include <string>

#include "operation.hpp"


namespace DBus {
namespace Object {

const std::string OperationString(const Operation &op)
{
    switch (op)
    {
    case Operation::NONE:
        return "NONE";
    case Operation::METHOD_CALL:
        return "METHOD_CALL";
    case Operation::PROPERTY_SET:
        return "PROPERTY_SET";
    case Operation::PROPERTY_GET:
        return "PROPERTY_GET";
    default:
        return "[UNKNOWN]";
    }
}

} // namespace Object
} // namespace DBus
