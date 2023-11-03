//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/callbacklink.cpp
 *
 * @brief Implementation of DBus::Object::CallbackLink.
 *
 */

#include <string>
#include <sstream>

#include "callbacklink.hpp"


namespace DBus {
namespace Object {


namespace _private {
static inline std::string compose_errorclass(AsyncProcess::Request::UPtr &req)
{
    std::stringstream s;
    s << "Object::CallbackLink{Request=" << req << "}";
    return s.str();
}
} // namespace _private



CallbackLink::Exception::Exception(const std::string &errm)
    : DBus::Exception("Object::CallbackLink", errm, nullptr)
{
}


CallbackLink::Exception::Exception(const std::string &sender,
                                   const std::string &path,
                                   const std::string &interf,
                                   const std::string &errm)
    : DBus::Exception("Object::CallbackLink(sender=" + sender
                          + ", path=" + path
                          + ", interface=" + interf,
                      errm,
                      nullptr)
{
}


CallbackLink::Exception::Exception(AsyncProcess::Request::UPtr &req,
                                   const std::string &errm)
    : DBus::Exception(_private::compose_errorclass(req), errm, nullptr)
{
}


///////////////////////////////////////////////////////////////////


AsyncProcess::Request::UPtr CallbackLink::NewObjectOperation(GDBusConnection *conn,
                                                             const std::string &sender,
                                                             const std::string &obj_path,
                                                             const std::string &intf_name)
{
    if (!request_pool)
    {
        throw CallbackLink::Exception(sender, obj_path, intf_name, "Request Pool is nullptr");
    }
    return AsyncProcess::Request::Create(conn, object, sender, obj_path, intf_name);
}


void CallbackLink::QueueOperation(AsyncProcess::Request::UPtr &req)
{
    if (!request_pool)
    {
        throw CallbackLink::Exception(req, "Request Pool is nullptr");
    }
    if (!req)
    {
        throw CallbackLink::Exception("AsyncProcess::Request is nullptr");
    }
    request_pool->PushCallback(req);
}


///////////////////////////////////////////////////////////////


CallbackLink::CallbackLink(Object::Base::Ptr dbus_object,
                           Manager::WPtr object_manager,
                           AsyncProcess::Pool::Ptr async_pool)
    : object(dbus_object), manager(object_manager), request_pool(async_pool)
{
}


} // namespace Object
} // namespace DBus
