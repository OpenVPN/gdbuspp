//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//


/**
 * @file async-process.cpp
 *
 * @brief Implementation of the C++ wrapper for the
 *        glib2 g_thread_pool interface and the AsyncProcess::Request class
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <memory>

#include "async-process.hpp"
#include "authz-request.hpp"
#include "exceptions.hpp"
#include "object/base.hpp"
#include "glib2/callbacks.hpp"


using namespace DBus;


AsyncProcess::Exception::Exception(const std::string &err)
    : DBus::Exception("AsyncProcess", err)
{
}



//
//  AsyncProcess::Request
//



AsyncProcess::Request::Request(GDBusConnection *gdbus_conn_,
                               std::shared_ptr<Object::Base> dbus_object_,
                               const std::string &sender_,
                               const DBus::Object::Path &object_path,
                               const std::string &interface)
    : dbusconn(gdbus_conn_),
      object(dbus_object_),
      sender(sender_)
{
    if ((object_path != object->GetPath())
        || (interface != object->GetInterface()))
    {
        throw AsyncProcess::Exception("Mismatch of object path/interface "
                                      "between object accessed and request");
    }
}


void AsyncProcess::Request::MethodCall(const std::string &meth,
                                       GVariant *prms,
                                       GDBusMethodInvocation *invoc) noexcept
{
    request_type = Object::Operation::METHOD_CALL;
    method = meth;
    params = prms;
    invocation = invoc;
}


void AsyncProcess::Request::GetProperty(const std::string &propname) noexcept
{
    request_type = Object::Operation::PROPERTY_GET;
    property = propname;
}


void AsyncProcess::Request::SetProperty(const std::string &propname, GVariant *prms) noexcept
{
    request_type = Object::Operation::PROPERTY_SET;
    property = propname;
    params = prms;
}



//
//   AsyncProcess::Pool
//



AsyncProcess::Pool::Pool()
{
    int avail_procs = (g_get_num_processors() / 2);
    pool = g_thread_pool_new(glib2::Callbacks::_int_pool_processpool_cb,
                             nullptr,
                             std::max(1, avail_procs),
                             false,
                             nullptr);
    if (!pool)
    {
        throw DBus::Exception("AsyncProcess::Pool", "g_thread_pool_new() failed");
    }
}


AsyncProcess::Pool::~Pool() noexcept
{
    g_thread_pool_free(pool, true, true);
}


void AsyncProcess::Pool::PushCallback(Request::UPtr &req)
{
    void *ptr = req.release();
    if (!ptr)
    {
        std::cerr << "ctx.release() returned nullptr" << std::endl;
        return;
    }


    GError *err = nullptr;

    if (!g_thread_pool_push(pool, ptr, &err))
    {
        if (err)
        {
            std::cerr << "** ERROR ** AsyncProcess::Pool::PushCallback:"
                      << std::string(err->message) << std::endl
                      << "         ** " << &req << std::endl;
        }
        else
        {
            std::cerr << "** ERROR **  AsyncProcess::Pool::PushCallback:"
                      << "Failed calling g_thread_pool_push() {" << &req << "}"
                      << std::endl;
        }
    }
}
