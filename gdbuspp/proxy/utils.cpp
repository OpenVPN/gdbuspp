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


Query::Ptr Query::Create(Proxy::Client::Ptr proxy)
{
    if (!proxy)
    {
        throw DBus::Proxy::Exception("Invalid DBus::Proxy::Client object");
    }

    return Query::Ptr(new Query(proxy));
}

const bool Query::Ping() const noexcept
{
    auto target = Proxy::TargetPreset::Create("/", "org.freedesktop.DBus.Peer");
    for (int i = 5; i > 0; --i)
    {
        try
        {
            GVariant *r = proxy->Call(target, "Ping", nullptr);
            g_variant_unref(r);
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
    for (int i = 15; i > 0; --i)
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
            if (err.find("No such interface") != std::string::npos)
            {
                return false;
            }
            usleep(100000);
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
                               "ServiceVersion::Ping",
                               "Could not reach the service");
    }

    // This method is typically called early on and the service
    // might not have settled yet, thus not responsive.
    // So we give it a chance up to 750ms to settle down before
    // declaring the service as inaccessible
    bool found = false;
    for (uint8_t i = 5; i > 0; i--)
    {
        found = CheckObjectExists(path, interface);
        if (found)
        {
            break;
        }
        usleep(150000);
    }
    if (!found)
    {
        throw Proxy::Exception(proxy->GetDestination(),
                               path,
                               interface,
                               "ServiceVersion::CheckObjectExists",
                               "Service is inaccessible");
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
                      "Failed querying service '" + service + "': " + msg)
{
}


DBusServiceQuery::Ptr DBusServiceQuery::Create(DBus::Connection::Ptr connection)
{
    if (!connection || !connection->Check())
    {
        throw DBus::Proxy::Exception("Invalid DBus::Connection object");
    }
    return DBusServiceQuery::Ptr(new DBusServiceQuery(connection));
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


const bool DBusServiceQuery::LookupService(const std::string &service) const
{
    try
    {
        GVariant *res = proxy->Call("/",
                                    "org.freedesktop.DBus",
                                    "ListNames",
                                    nullptr);
        auto list = glib2::Value::ExtractVector<std::string>(res, 0);

        for (const auto &srv : list)
        {
            if (srv == service)
            {
                return true;
            }
        }
        return false;
    }
    catch (const Proxy::Exception &excp)
    {
        throw DBusServiceQuery::Exception(service, excp.GetRawError());
    }
}


const bool DBusServiceQuery::LookupActivatable(const std::string &service) const
{
    GVariant *res = proxy->Call("/",
                                "org.freedesktop.DBus",
                                "ListActivatableNames",
                                nullptr);
    auto list = glib2::Value::ExtractVector<std::string>(res, 0);

    for (const auto &srv : list)
    {
        if (srv == service)
        {
            return true;
        }
    }
    return false;
}


const bool DBusServiceQuery::CheckServiceAvail(const std::string &service,
                                               uint8_t timeout) const noexcept
{
    // Check if the service is already running
    if (LookupService(service))
    {
        return true;
    }

    // If not, identify if this service can be started via the
    // org.freedesktop.DBus.StartServiceByName() call to the D-Bus daemon
    bool activatable = true;
    try
    {
        activatable = LookupActivatable(service);
    }
    catch (const DBus::Exception &)
    {
        // If this lookup failed, presume it is possible to activate the
        // service.  If it is completely impossible to start the service,
        // that will be caught in by the StartServiveByName() call below.
    }

    int iterations = timeout * 3; // 3 iterations takes about 1 second
    for (int i = iterations; i > 0; --i)
    {
        try
        {
            // If the service is listed as an activatable service,
            // The org.freedesktop.DBus.StartServiceByName() can be called
            // without getting an error.  This indicates the service has
            // a /usr/share/dbus-1/{services,system-services}/*.service file
            // configured.
            if (activatable)
            {
                (void)StartServiceByName(service);
            }

            if (LookupService(service))
            {
                return true;
            }
            usleep(300000);
        }
        catch (const DBusServiceQuery::Exception &)
        {
            if (!activatable)
            {
                // Quick exit - if the service can't be auto-started
                // via org.freedesktop.DBus.StartServiceByName(), there
                // are no point retrying the same loop more times.
                return false;
            }

            // If the service can be auto-started, the GetNameOwner()
            // call failed - which can mean the service did respond to
            // the auto-start request but hasn't settled yet - just wait
            // a little bit before retrying.
            usleep(300000);
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
