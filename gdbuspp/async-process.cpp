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
#include "features/fmt-compat.hpp"
#include <fmt/format.h>
#include <fmt/printf.h>
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


std::string AsyncProcess::Request::str() const
{
    std::string op;
    switch (request_type)
    {
    case Object::Operation::METHOD_CALL:
        op = fmt::format(FMT_COMPILE(", method={}"), method);
        break;
    case Object::Operation::PROPERTY_SET:
    case Object::Operation::PROPERTY_GET:
        op = fmt::format(FMT_COMPILE(", property={}"), property);
        break;
    case Object::Operation::NONE:
        break;
    }

    return fmt::format(FMT_COMPILE("{{AsyncPool::Request {}: Type={}{}}}"),
                       object,
                       Object::OperationString(request_type),
                       op);
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
        fmt::print(stderr, "ctx.release() returned nullptr\n");
        return;
    }


    GError *err = nullptr;

    if (!g_thread_pool_push(pool, ptr, &err))
    {
        if (err)
        {
            fmt::print(stderr,
                       FMT_COMPILE("** ERROR ** AsyncProcess::Pool::PushCallback: {}\n"
                                   "         ** {}\n"),
                       err->message,
                       req);
        }
        else
        {
            fmt::print(stderr,
                       FMT_COMPILE("** ERROR **  AsyncProcess::Pool::PushCallback: "
                                   "Failed calling g_thread_pool_push() {{{}}}\n"),
                       req);
        }
    }
}
