//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once

/**
 *  @file credentials/query.hpp
 *
 *  @brief Additional tool to retrieve connection credential details
 *         about a specific D-Bus service
 */


#include "../connection.hpp"
#include "../proxy.hpp"
#include "../glib2/utils.hpp"

#include "exceptions.hpp"


namespace DBus {
namespace Credentials {

/**
 *   Queries the D-Bus daemon for the credentials of a specific D-Bus
 *   bus name.  Each D-Bus client performing an operation on a D-Bus
 *   object in a service connects with a unique bus name.  This is a
 *   safe method for retrieving information about who the caller is.
 */
class Query
{
  public:
    using Ptr = std::shared_ptr<Credentials::Query>;

    /**
     *  Create a new DBus::Credentials::Query object
     *
     * @param dbuscon   DBus::Connection object where queries will be performed
     *
     * @return Credentials::Query::Ptr
     *
     */
    [[nodiscard]] static Credentials::Query::Ptr Create(DBus::Connection::Ptr dbuscon)
    {
        return Credentials::Query::Ptr(new Credentials::Query(dbuscon));
    }

    /**
     *  Retrieve the UID of the owner of a specific bus name
     *
     * @param  busname  String containing the bus name
     *
     * @return Returns an uid_t containing the bus owners UID.
     * @throws DBus::Credentials::Exception on errors     */
    const uid_t GetUID(const std::string &busname) const;

    /**
     *  Retrieves the bus name callers process ID (PID) of a specific
     *  bus name
     *
     * @param busname  String containing the bus name for the query
     *
     * @return Returns a pid_t value of the process ID.
     * @throws DBus::Credentials::Exception on errors
     *
     */
    const pid_t GetPID(const std::string &busname) const;

    /**
     *  Looks up the unique D-Bus bus name for a given D-Bus service,
     *  based on a well-known bus name.
     *
     * @param busname A string containing the well known bus name.
     *
     * @return Returns a string containing the unique bus name if found,
     * @throws DBus::Credentials::Exception on errors
     *
     */
    const std::string GetUniqueBusName(const std::string &busname) const;


  private:
    DBus::Proxy::Client::Ptr dbus_proxy = nullptr;
    DBus::Proxy::TargetPreset::Ptr dbus_target = nullptr;


    /**
     * Initiate the object for querying the D-Bus daemon.  This is always
     * the org.freedesktop.DBus service and the interface is also the
     * same value.  Object path is not used by this service.
     *
     * @param dbuscon  An established D-Bus connection to use for queries
     *
     */
    Query(DBus::Connection::Ptr dbuscon);
};


} // namespace Credentials
} // namespace DBus
