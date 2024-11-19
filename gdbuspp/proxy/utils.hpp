//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file proxy/utils.hpp
 *
 * @brief  DBus::Proxy::Client utilities.  These features
 *         requires a DBus::Proxy::Client or a DBus::Connection to be
 *         prepared in advance
 */

#pragma once

#include <memory>
#include <string>

#include "../connection.hpp"
#include "../object/path.hpp"
#include "../proxy.hpp"


namespace DBus {
namespace Proxy {
namespace Utils {


/**
 *  Provides some generic functionality to test if a D-Bus service is
 *  available and running, if specific D-Bus objects exists in that service,
 *  can extract the 'version' property in the service object (service must
 *  be designed to provide this property) and retrieve the Introspection
 *  data from a service.
 */
class Query
{
  public:
    using Ptr = std::shared_ptr<Query>;

    /**
     *  Prepare a Proxy::Utils::Query object.  It requires a configured
     *  DBus::Proxy::Client object for the service it will query
     *
     * @param proxy          DBus::Proxy::Client object where query will
     *                       be directed
     * @return Query::Ptr
     */
    [[nodiscard]] static Query::Ptr Create(Proxy::Client::Ptr proxy);

    /**
     *  Calls the org.freedesktop.DBus.Peer.Ping method, to check if the
     *  the service is alive.
     *
     * @return true if service is available and responding, otherwise false
     */
    const bool Ping() const noexcept;

    /**
     *  Calls the org.freedesktop.DBus.Introspectable.Introspect method
     *  and returns the XML introspection data as a plain string.
     *
     * @param path    D-Bus::Object::Path to introspect in the service
     *
     * @return const std::string  XML introspection data
     */
    const std::string Introspect(const Object::Path &path) const;

    /**
     *  Does a few checks to try to reach a D-Bus object in the service.
     *
     * @param path        D-Bus object path to look for
     * @param interface   D-Bus interface in the object
     *
     * @return true if the object exists and is reachable, otherwise false
     */
    const bool CheckObjectExists(const Object::Path &path,
                                 const std::string interface) const noexcept;


    /**
     *  Retrieve the service version, based on the 'version' property in
     *  D-Bus object path provided.
     *
     * @param path                D-Bus::Object::Path where to find the version
     *                            information in the service
     * @param interface           D-Bus object interface holding the 'version'
     *                            property
     *
     * @return const std::string containing the 'version' property value
     */
    const std::string ServiceVersion(const Object::Path &path,
                                     const std::string interface) const;

  private:
    Proxy::Client::Ptr proxy{nullptr};

    Query(Proxy::Client::Ptr proxy_);
};



/**
 *  A generic set of selected methods provided by the org.freedesktop.DBus
 *  service.
 */
class DBusServiceQuery
{
  public:
    using Ptr = std::shared_ptr<DBusServiceQuery>;

    class Exception : public DBus::Exception
    {
      public:
        Exception(const std::string &service, const std::string &msg);
    };


    /**
     *  Prepare a DBusServiceQuery object
     *
     * @param connection   DBus::Connection::Ptr with the bus connection to use
     *
     * @return DBusServiceQuery::Ptr
     */
    [[nodiscard]] static DBusServiceQuery::Ptr Create(DBus::Connection::Ptr connection);

    /**
     *  Calls the org.freedesktop.DBus.StartServiceByName method
     *
     *  https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-start-service-by-name
     *
     * @param service          std::string to the service to start
     *
     * @return const uint32_t  Returns 1 if the service was started, 2 if the
     *         service was already running.
     *
     * @throws DBusServiceQuery::Exception on errors
     */
    const uint32_t StartServiceByName(const std::string &service) const;

    /**
     *  Retrieve the unique bus name for the given service name
     *  This functionality is also available via the
     *  DBus::Credentials::Query::GetUniqueBusName() method
     *
     *  https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-name-owner
     *
     * @param service              D-Bus service name to query for
     * @return const std::string containing a unqiue bus name of the service
     */
    const std::string GetNameOwner(const std::string &service) const;


    /**
     *  Looks up a specific busname of a service to see if it
     *  is running.  This will use the org.freedesktop.DBus.ListNames method
     *  for the lookup search.
     *
     * @param service  std::string with the bus name of the service.
     * @return true if the the service was found enlisted, otherwise false.
     */
    const bool LookupService(const std::string &service) const;


    /**
     *  Looks up a specific busname of a service to see if it
     *  is can be activated by the D-Bus daemon on the system.  This will
     *  use the org.freedesktop.DBus.ListActivatableNames method
     *  for the lookup search.
     *
     * @param service  std::string with the bus name of the service.
     * @return true if the the service was found enlisted, otherwise false.
     */
    const bool LookupActivatable(const std::string &service) const;


    /**
     *  A more vigorous attempt of ensuring that a D-Bus service is alive and
     *  available.  This will try 5 times before giving up, sleeping for a short
     *  while in between each attempt.

     * @param service   D-Bus service well-known bus name to check for
     * @param timeout   How many seconds to wait for the service to respond.
     *                  Defaults to approx. 10 seconds.
     * @return true if the service is available, otherwise false
     */
    const bool CheckServiceAvail(const std::string &service,
                                 uint8_t timeout = 10) const noexcept;


  private:
    Proxy::Client::Ptr proxy{nullptr};

    DBusServiceQuery(DBus::Connection::Ptr connection);
};

} // namespace Utils
} // namespace Proxy
} // namespace DBus
