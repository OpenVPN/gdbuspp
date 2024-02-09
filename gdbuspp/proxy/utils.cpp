//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file proxy/utils.cpp
 *
 * @brief  Implementation of DBus::Proxy::Client utilities.
 */


#include "../object/path.hpp"
#include "utils.hpp"


namespace DBus {
namespace Proxy {
namespace Utils {


const bool Query::Ping() const noexcept
{
    auto target = Proxy::TargetPreset::Create("/", "org.freedesktop.DBus.Peer");
    for (int i = 5; i > 0; --i)
    {
        try
        {
            proxy->Call(target, "Ping", nullptr, true);
            return true;
        }
        catch (const Proxy::Exception &excp)
        {
            usleep(300000);
        }
    }
    return false;
}


const std::string Query::Introspect(const Object::Path &path) const
{
    GVariant *res = proxy->Call(path,
                                "org.freedesktop.DBus.Introspectable",
                                "Introspect",
                                nullptr);
    std::string ret = glib2::Value::Extract<std::string>(res, 0);
    g_variant_unref(res);
    return ret;
}


const bool Query::CheckObjectExists(const Object::Path &path,
                                    const std::string interface) const noexcept
{
    auto target = Proxy::TargetPreset::Create(path,
                                              "org.freedesktop.DBus.Properties");
    for (int i = 4; i > 0; --i)
    {
        try
        {
            GVariant *ignore = proxy->Call(target,
                                           "GetAll",
                                           glib2::Value::CreateTupleWrapped(interface));
            if (ignore)
            {
                g_variant_unref(ignore);
            }
            return true;
        }
        catch (const Proxy::Exception &excp)
        {
            std::string err(excp.what());
            if (err.find("Name \"") != std::string::npos
                && err.find("\" does not exist") != std::string::npos)
            {
                return false;
            }
            usleep(300);
        }
    }
    return false;
}


const std::string Query::ServiceVersion(const Object::Path &path,
                                        const std::string interface) const
{
    if (!Ping())
    {
        throw Proxy::Exception(proxy->GetDestination(),
                               path,
                               interface,
                               "Could not reach the service");
    }

    if (!CheckObjectExists(path, interface))
    {
        throw Proxy::Exception(proxy->GetDestination(),
                               path,
                               interface,
                               "Object path/interface does not exists");
    }

    auto target = Proxy::TargetPreset::Create(path, interface);
    return proxy->GetProperty<std::string>(target, "version");
}


Query::Query(Proxy::Client::Ptr proxy_)
    : proxy(proxy_)
{
}



/////////////////////////////////////////////////////////////////



DBusServiceQuery::Exception::Exception(const std::string &service, const std::string &msg)
    : DBus::Exception("Proxy::Utils::DBusQuery",
                      "Failed querying service '" + service + "':" + msg)
{
}



const uint32_t DBusServiceQuery::StartServiceByName(const std::string &service) const
{
    try
    {
        GVariant *res = proxy->Call("/",
                                    "org.freedesktop.DBus",
                                    "StartServiceByName",
                                    g_variant_new("(su)", service.c_str(), 0));
        auto ret = glib2::Value::Extract<uint32_t>(res, 0);
        g_variant_unref(res);
        return ret;
    }
    catch (const Proxy::Exception &excp)
    {
        throw DBusServiceQuery::Exception(service, excp.GetRawError());
    }
}


const std::string DBusServiceQuery::GetNameOwner(const std::string &service) const
{
    try
    {
        GVariant *res = proxy->Call("/",
                                    "org.freedesktop.DBus",
                                    "GetNameOwner",
                                    glib2::Value::CreateTupleWrapped(service));
        auto ret = glib2::Value::Extract<std::string>(res, 0);
        g_variant_unref(res);
        return ret;
    }
    catch (const Proxy::Exception &excp)
    {
        throw DBusServiceQuery::Exception(service, excp.GetRawError());
    }
}


const bool DBusServiceQuery::CheckServiceAvail(const std::string &service) const noexcept
{
    for (int i = 5; i > 0; --i)
    {
        try
        {
            uint32_t res = StartServiceByName(service);
            if (2 == res) // DBUS_START_REPLY_ALREADY_RUNNING
            {
                return true;
            }
            (void)GetNameOwner(service);
            return true;
        }
        catch (const DBusServiceQuery::Exception &)
        {
            usleep(400000);
        }
    }
    return false;
}


DBusServiceQuery::DBusServiceQuery(DBus::Connection::Ptr connection)
    : proxy(Proxy::Client::Create(connection, "org.freedesktop.DBus"))
{
}


} // namespace Utils
} // namespace Proxy
} // namespace DBus
