//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file authz-request.cpp
 *
 * @brief  Implementation of the Authz::Request and related APIs
 */

#include <string>
#include <fmt/compile.h>
#include <fmt/format.h>

#include "authz-request.hpp"
#include "object/base.hpp"
#include "object/operation.hpp"
#include "object/path.hpp"


namespace DBus {
namespace Authz {

Exception::Exception(const Request::Ptr req, const std::string &errmsg)
    : DBus::Exception("AuthzReq", compose_error(req, errmsg))
{
}


std::string Exception::compose_error(const Request::Ptr req,
                                     const std::string &errmsg)
{
    if (!errmsg.empty())
    {
        return errmsg;
    }

    std::string operation;
    switch (req->operation)
    {
    case Object::Operation::METHOD_CALL:
        operation = "performing method call";
        break;
    case Object::Operation::PROPERTY_GET:
        operation = "reading property";
        break;
    case Object::Operation::PROPERTY_SET:
        operation = "setting property";
        break;
    case Object::Operation::NONE:
        operation = "[NO OPERATION]";
    }
    return fmt::format(FMT_COMPILE("Authorization failed for {} {} {} in object {}"),
                       req->caller,
                       operation,
                       req->target,
                       req->object_path);
}



/**
 *  Internal helper function to extract an operation target string from
 *  an AsyncProcess::Request object
 *
 *  This is basically just helping the Authz::Request constructor to
 *  extract information which can be stored as constant variables.
 *
 *  The result will be a concatenated string of the D-Bus interface
 *  plus the D-Bus method or property being accessed.
 *
 * @param reqtype    Object::Operation request type
 * @param interface  D-Bus interface being accessed
 * @param method     D-Bus method being called; related to the request type
 * @param property   D-Bus property being accessed, related to the request type
 * @return std::string with readable authorization request information
 */
static inline std::string extract_authzreq_target(const Object::Operation reqtype,
                                                  const std::string &interface,
                                                  const std::string &method,
                                                  const std::string &property)
{
    std::string detail;
    switch (reqtype)
    {
    case Object::Operation::METHOD_CALL:
        detail = method;
        break;
    case Object::Operation::PROPERTY_GET:
    case Object::Operation::PROPERTY_SET:
        detail = property;
        break;
    default:
        throw DBus::Exception("Authz::Request::Create",
                              "Invalid request type conversion");
    }
    return fmt::format(FMT_COMPILE("{}.{}"), interface, detail);
    ;
}


Authz::Request::Request(const std::string &caller_,
                        const Object::Operation operation_,
                        const Object::Path &object_path_,
                        const std::string &interace_,
                        const std::string &target_)
    : caller(caller_), operation(operation_),
      object_path(object_path_), interface(interace_), target(target_)
{
}


Authz::Request::Request(const AsyncProcess::Request::UPtr &req)
    : caller(req->sender),
      operation(req->request_type),
      object_path(req->object->GetPath()),
      interface(req->object->GetInterface()),
      target(extract_authzreq_target(req->request_type,
                                     req->object->GetInterface(),
                                     req->method,
                                     req->property))
{
}


std::string Authz::Request::OperationString() const noexcept
{
    return Object::OperationString(operation);
}


std::string Authz::Request::str() const
{
    return fmt::format(
        FMT_COMPILE("AuthzRequest(caller={}, operation={}, path={}, interface={}, target={})"),
        caller,
        OperationString(),
        object_path,
        interface,
        target);
}


} // namespace Authz
} // namespace DBus
