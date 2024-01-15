//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 *  @file credentials/query.cpp
 *
 *  @brief Implementation of DBus::Credential::Service
 */

#include "exceptions.hpp"
#include "query.hpp"


namespace DBus {
namespace Credentials {


Credentials::Query::Query(DBus::Connection::Ptr dbuscon)
{
    // The information we want to request is provided by the
    // org.freedesktop.DBus service
    dbus_proxy = Proxy::Client::Create(dbuscon, "org.freedesktop.DBus");
    dbus_target = Proxy::TargetPreset::Create("/net/freedesktop/DBus",
                                              "org.freedesktop.DBus");
}


const uid_t Credentials::Query::GetUID(const std::string &busname) const
{
    try
    {
        GVariant *result = dbus_proxy->Call(dbus_target,
                                            "GetConnectionUnixUser",
                                            glib2::Value::CreateTupleWrapped(busname));
        uid_t ret = glib2::Value::Extract<uint32_t>(result, 0);
        g_variant_unref(result);
        return ret;
    }
    catch (const DBus::Exception &excp)
    {
        throw Credentials::Exception("GetUID",
                                     "Failed to retrieve UID for bus name '"
                                         + busname + "': " + excp.GetRawError());
    }
}


const pid_t Credentials::Query::GetPID(const std::string &busname) const
{
    try
    {
        GVariant *result = dbus_proxy->Call(dbus_target,
                                            "GetConnectionUnixProcessID",
                                            glib2::Value::CreateTupleWrapped(busname));
        pid_t pid = glib2::Value::Extract<uint32_t>(result, 0);
        g_variant_unref(result);
        return pid;
    }
    catch (const DBus::Exception &excp)
    {
        throw Credentials::Exception("GetPID",
                                     "Failed to retrieve process ID for bus name '"
                                         + busname + "': " + excp.GetRawError());
    }
}


const std::string Credentials::Query::GetUniqueBusName(const std::string &busname) const
{
    try
    {
        GVariant *result = dbus_proxy->Call(dbus_target,
                                            "GetNameOwner",
                                            glib2::Value::CreateTupleWrapped(busname));
        std::string ret = glib2::Value::Extract<std::string>(result, 0);
        g_variant_unref(result);
        return ret;
    }
    catch (const DBus::Exception &excp)
    {
        throw Credentials::Exception("GetPID",
                                     "Failed to retrieve unique bus name for '"
                                         + busname + "': " + excp.GetRawError());
    }
}

} // namespace Credentials
} // namespace DBus
