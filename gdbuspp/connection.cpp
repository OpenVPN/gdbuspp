//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file connection.cpp
 *
 * @brief Implementation of the DBus::Connection class
 */

#include <iostream>
#include <string>
#include <glib.h>
#include <gio/gio.h>

#include "connection.hpp"

namespace DBus {


Connection::Exception::Exception(const std::string &err, GError *gliberr)
    : DBus::Exception("DBus::Connection", err, gliberr)
{
}


Connection::Connection(BusType bustype)
    : type(bustype)
{
    GBusType glib2_bustype = G_BUS_TYPE_NONE;
    switch (bustype)
    {
    case BusType::SESSION:
        glib2_bustype = G_BUS_TYPE_SESSION;
        break;
    case BusType::SYSTEM:
        glib2_bustype = G_BUS_TYPE_SYSTEM;
        break;
    default:
        throw Connection::Exception("Invalid bus type");
    }

    GError *error = nullptr;
    dbuscon = g_bus_get_sync(glib2_bustype, nullptr, &error);
    if (!dbuscon || error)
    {
        std::string errmsg = "Could not connect to the D-Bus";
        if (error)
        {
            errmsg += ": " + std::string(error->message);
        }
        dbuscon = nullptr;
        throw Connection::Exception(errmsg);
    }
}


Connection::Connection(GDBusConnection *dbuscon)
    : dbuscon(dbuscon), type(BusType::UNKNOWN)
{
    if (!G_IS_DBUS_CONNECTION(dbuscon))
    {
        throw Connection::Exception("Invalid connection pointer");
    }
}


Connection::~Connection() noexcept
{
    try
    {
        Connection::Disconnect();
    }
    catch (const Connection::Exception &err)
    {
        std::cerr << std::string(err.what()) << std::endl;
    }
}


GDBusConnection *Connection::ConnPtr() const noexcept
{
    return dbuscon;
}


const std::string Connection::GetUniqueBusName() const
{
    if (!G_IS_DBUS_CONNECTION(dbuscon))
    {
        throw Connection::Exception("Invalid connection");
    }
    return std::string(g_dbus_connection_get_unique_name(dbuscon));
}


const bool Connection::Check() const
{
    return dbuscon
           && G_IS_DBUS_CONNECTION(dbuscon)
           && !g_dbus_connection_is_closed(dbuscon);
}


void Connection::Disconnect()
{
    if (!dbuscon)
    {
        return;
    }
    if (!G_IS_DBUS_CONNECTION(dbuscon))
    {
        throw Connection::Exception("D-Bus connection broken");
    }

    GError *err_flush = nullptr;
    bool r = g_dbus_connection_flush_sync(dbuscon, nullptr, &err_flush);
    if (!r || err_flush)
    {
        throw Connection::Exception("Connection flush failed", err_flush);
    }

    GError *err = nullptr;
    g_dbus_connection_close_sync(dbuscon, nullptr, &err);
    if (err)
    {
        throw Connection::Exception("D-Bus disconnect failed", err);
    }
    g_object_unref(dbuscon);
    dbuscon = nullptr;
}

} // namespace DBus
