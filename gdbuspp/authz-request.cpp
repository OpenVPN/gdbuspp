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


const std::string Exception::compose_error(const Request::Ptr req,
                                           const std::string &errmsg)
{
    if (!errmsg.empty())
    {
        return errmsg;
    }

    std::ostringstream r;
    r << "Autorization failed for " << req->caller;
    switch (req->operation)
    {
    case Object::Operation::METHOD_CALL:
        r << " performing method call ";
        break;
    case Object::Operation::PROPERTY_GET:
        r << " reading property ";
        break;
    case Object::Operation::PROPERTY_SET:
        r << " setting property ";
        break;
    case Object::Operation::NONE:
        r << " [NO OPERATION] ";
    }
    r << req->target << " in object " << req->object_path;
    return std::string(r.str());
}



/**
 *  Internal helper function to extract an operation target string from
 *  an AsyncProcess::Request object
 *
 *  This is basically just helping the Authz::Request constructor to
 *  extract information which can be stored as constant variables

 * @param t    AsyncProcess::ReqType value to conviert
 * @return std::string with readable authorization request information
 */
static inline const std::string extract_authzreq_target(const Object::Operation reqtype,
                                                        const std::string &interface,
                                                        const std::string &method,
                                                        const std::string &property)
{
    std::string trgt = interface + ".";
    switch (reqtype)
    {
    case Object::Operation::METHOD_CALL:
        trgt += method;
        break;
    case Object::Operation::PROPERTY_GET:
    case Object::Operation::PROPERTY_SET:
        trgt += property;
        break;
    default:
        throw DBus::Exception("Authz::Request::Create",
                              "Invalid request type conversion");
    }
    return trgt;
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


const std::string Authz::Request::OperationString() const noexcept
{
    return Object::OperationString(operation);
}

} // namespace Authz
} // namespace DBus
