//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file authz-request.hpp
 *
 * @brief  Declaration of the Authz Request API
 */

#pragma once

#include <string>

#include "async-process.hpp"
#include "exceptions.hpp"
#include "object/operation.hpp"

namespace DBus {

/**
 *   D-Bus Authorization
 */
namespace Authz {

class Exception; // forward delcaration

/**
 *  Authz Request
 *
 *  Before each method or set/get property callback is processed,
 *  an Authz request is sent to the object the D-Bus call
 *  is targeting.  The DBus::Object::Authorize() method receives
 *  this request and it need to evaluate the request before returning
 *  a true/false response to allow or reject the D-Bus call.
 *
 */
class Request
{
  public:
    friend class Authz::Exception;
    using Ptr = std::shared_ptr<Authz::Request>;


    /**
     *  Create a new Authz Request
     *
     * @param caller       std::string containing the D-Bus callers
     *                     unique bus id
     * @param operation    Object::Operation with the type of call
     * @param object_path  D-Bus object this call is targeting
     * @param interface    D-Bus interface this call is targeting
     * @param target       The D-Bus object's method or property being
     *                     targeted
     *
     * @return Authz::Reqeuest::Ptr  A new object which can be passed to
     *         the DBus::Object::Authorize() method
     */
    [[nodiscard]] static Request::Ptr Create(const std::string &caller,
                                             const Object::Operation operation,
                                             const std::string &object_path,
                                             const std::string &interface,
                                             const std::string &target)
    {
        return Ptr(new Request(caller, operation, object_path, interface, target));
    }


    /**
     *  Construct a new Authz Request object by extracting information
     *  from a AsyncProcess::Request object.
     *
     * @param req   The AsyncProcess::Request object to extract information from
     *
     * @return Authz::Reqeuest::Ptr  A new object which can be passed to
     *         the DBus::Object::Authorize() method
     */
    [[nodiscard]] static Request::Ptr Create(const AsyncProcess::Request::UPtr &req)
    {
        return Ptr(new Request(req));
    }


    /**
     *  Retrieve this object's operation mode as a human readable string
     *
     * @return const std::string
     */
    const std::string OperationString() const noexcept;

    /**
     *  Standard iostream compliant stream operator, providing a human readable
     *  representation of the important information carried by this Request object.
     *
     *  This is mostly useful in error reporting and debugging contexts.
     */
    friend std::ostream &operator<<(std::ostream &os, const Request::Ptr req)
    {
        std::ostringstream r;
        r << "AuthzRequest("
          << "caller=" << req->caller << ", "
          << "operation=" << req->OperationString() << ", "
          << "path=" << req->object_path << ", "
          << "interface=" << req->interface << ", "
          << "target=" << req->target << ")";
        return os << r.str();
    }


  private:
    const std::string caller;          ///< D-Bus unique bus ID of the caller
    const Object::Operation operation; ///< Operation requested by caller
    const std::string object_path;     ///< D-Bus object path caller wants to access
    const std::string interface;       ///< D-Bus interface of the object
    const std::string target;          ///< Method/Property caller wants to access


    /**
     * @see Create
     */
    Request(const std::string &caller_,
            const Object::Operation operation_,
            const std::string &object_path_,
            const std::string &interace_,
            const std::string &target_);

    /**
     * @see Create
     */
    Request(const AsyncProcess::Request::UPtr &req);
};



class Exception : public DBus::Exception
{
  public:
    Exception(const Authz::Request::Ptr req);

  private:
    const std::string compose_error(const Request::Ptr req);
};

}; // namespace Authz
} // namespace DBus
