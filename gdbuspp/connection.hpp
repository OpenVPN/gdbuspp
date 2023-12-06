//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once

#include <map>
#include <memory>
#include <string>
#include <glib.h>
#include <gio/gio.h>

#include "exceptions.hpp"

/**
 * @file connection.cpp
 *
 * @brief Declaration of the DBus::Connection class
 */

namespace DBus {

/**
 *  Supported D-Bus bus types in this implementation
 *
 */
enum class BusType
{
    UNKNOWN, ///< Not identified/set
    SESSION, ///< Connect to the Session D-Bus bus
    SYSTEM   ///< Connect to the System D-Bus bus
};

/**
 *  The DBus::Connection object manages the low-level connection
 *  to the main D-Bus daemon on the system.
 */
class Connection
{
  public:
    using Ptr = std::shared_ptr<Connection>;

    class Exception : public DBus::Exception
    {
      public:
        Exception(const std::string &err, GError *gliberr = nullptr);
        virtual ~Exception() noexcept = default;
    };


    /**
     *  Prepares a new connection to the D-Bus bus.
     *
     * @param bustype  Defines if the connection should be to the
     *                 system bus or session bus
     * @return Returns a Connection::Ptr with the requested D-Bus connection
     */
    [[nodiscard]] static Connection::Ptr Create(const DBus::BusType &bustype)
    {
        return Ptr(new Connection(bustype));
    }


    /**
     *  Prepares a new DBus::Connection object based on an already
     *  ready glib2 raw connection pointer
     *
     * @param dbuscon  glib2 GDBusConnection object to use
     *
     * @return Returns a Connection::Ptr with the requested D-Bus connection
     */
    Connection(GDBusConnection *dbuscon);

    /**
     * When the Connection object is being destructed, it will first
     * close all connections and release related resources.
     */
    virtual ~Connection() noexcept;


    /**
     *  Retrieve the raw glib2 based connection pointer
     *
     * @return GDBusConnection*
     */
    GDBusConnection *ConnPtr() const noexcept;


    /**
     *  Retrieve the unique D-Bus bus name this connection has been
     *  assigned.  This is controlled by the main D-Bus daemon on the
     *  system.
     *
     * @return const std::string
     */
    const std::string GetUniqueBusName() const;

    /**
     *  Check if the D-Bus connection is still valid
     *
     * @return Returns true if the connection is still valid, otherwise false
     */
    const bool Check() const;

    /**
     *  Explicit disconnect request, clossing the D-Bus connection
     *  and release related resources.
     */
    void Disconnect();

    friend std::ostream &operator<<(std::ostream &os, const Connection::Ptr &req)
    {
        switch (req->type)
        {
        case BusType::SESSION:
            return os << std::string("Connection(BusType::SESSION)");
        case BusType::SYSTEM:
            return os << std::string("Connection(BusType::SYSTEM)");
        default:
            return os << std::string("Connection(BusType::UNKNOWN)");
        }
    }



  private:
    GDBusConnection *dbuscon = nullptr; ///< The glib2 connection object
    BusType type;                       ///< The bus type this object uses

    Connection(DBus::BusType bustype);
};

} // namespace DBus
