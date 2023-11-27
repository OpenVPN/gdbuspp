//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//


/**
 * @file proxy.hpp
 *
 * @brief Declaration of a base DBus proxy class, used to interact
 *        with a D-Bus service from C++
 */

#pragma once

#include <iostream> // DEBUG: Remove with std::cout
#include <memory>
#include <glib.h>

#include "connection.hpp"
#include "exceptions.hpp"
#include "glib2/utils.hpp"


namespace DBus {
namespace Proxy {

class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &errm, GError *gliberr = nullptr);
    Exception(const std::string &destination,
              const std::string &path,
              const std::string &interface,
              const std::string &errm,
              GError *gliberr = nullptr);

  private:
    const std::string compose_error(const std::string &destination,
                                    const std::string &path,
                                    const std::string &interface) const;
};



/**
 *  Helper class for @Proxy::Client::Call()
 *  This contains a preset for a specific D-Bus object path and interface
 */
class TargetPreset
{
  public:
    using Ptr = std::shared_ptr<TargetPreset>;


    /**
     *  Prepare a new Proxy target

     * @param object_path_  std::string with the D-Bus object path of the target
     * @param interface_    std::string with the object interface in the target
     */
    [[nodiscard]] static Ptr Create(const std::string &object_path_,
                                    const std::string &interface_)
    {
        return Ptr(new TargetPreset(object_path_, interface_));
    }

    ~TargetPreset() = default;

    const std::string object_path;
    const std::string interface;

    friend std::ostream &operator<<(std::ostream &os, const TargetPreset *trgt)
    {
        return os << "object_path=" << trgt->object_path << ", "
                  << "interface=" << trgt->interface;
    };

  private:
    TargetPreset(const std::string &object_path_,
                 const std::string &interface_);
};



/**
 *   D-Bus Proxy client
 *
 *   This is a bare minimum client which can call D-Bus methods and
 *   interact with D-Bus object properties
 */
class Client
{
  public:
    using Ptr = std::shared_ptr<Client>;

    /**
     *  Prepare a new proxy client.  It will need an existing D-Bus
     *  connection and the service name to connect to
     *
     * @param connection   DBus::Connection
     * @param destination  std::string containing the D-Bus service to
     *                     connect to
     *
     * @return Client::Ptr Returns a shared_ptr with the prepared Proxy client
     */
    [[nodiscard]] static Client::Ptr Create(Connection::Ptr connection,
                                            const std::string &destination)
    {
        return Client::Ptr(new Client(connection, destination));
    }
    ~Client() noexcept = default;


    /**
     *  Retrieve the destination this proxy is configured to access
     *
     * @return const std::string& to the D-Bus destination name
     */
    const std::string &GetDestination() const noexcept;

    /**
     *  Call a D-Bus method in a D-Bus object on the D-Bus service this
     *  proxy is configured against

     * @param object_path  std::string with the D-Bus object path
     * @param interface    std::string with the interface scope in the object
     * @param method       std::string with the D-Bus method to call
     * @param params       GVariant * with the arguments to used in the
     *                     D-Bus method call
     * @param no_response  bool flag (default false). If set to true, don't
     *                     wait for a response.  This will also disable any
     *                     error checking possibilities, to verify if the
     *                     call was successfully executed by the service.
     *
     * @return GVariant*   GVariant object with the results provided by the
     *                     D-Bus method.  Will be nullptr if no_response is
     *                     set to to true.
     */
    GVariant *Call(const std::string &object_path,
                   const std::string &interface,
                   const std::string &method,
                   GVariant *params = nullptr,
                   const bool no_response = false) const;

    /**
     *  A variant of the prior @Proxy::Client::Call() method which
     *  extracts the D-Bus object path and interface from a TargetPreset
     *  object.
     *
     * @param preset       TargetPreset::Ptr containing the object path
     *                     and interface to perform the method call against
     * @param method       std::string with the D-Bus method to call
     * @param params       GVariant * with the arguments to used in the
     *                     D-Bus method call
     * @param no_response  bool flag (default false). If set to true, don't
     *                     wait for a response.  This will also disable any
     *                     error checking possibilities, to verify if the
     *                     call was successfully executed by the service.
     *
     * @return GVariant*   GVariant object with the results provided by the
     *                     D-Bus method.  Will be nullptr if no_response is
     *                     set to to true.
     */
    GVariant *Call(const TargetPreset::Ptr preset,
                   const std::string &method,
                   GVariant *params = nullptr,
                   const bool no_response = false) const;

    /**
     *  Do a D-Bus call and attach a file descriptor to be sent together
     *  with the call.  The D-Bus service will then get access to whatever
     *  this file descriptor accesses.

     * @param preset       TargetPreset::Ptr containing the object path
     *                     and interface to perform the method call against
     * @param method       std::string with the D-Bus method to call
     * @param params       GVariant * with the arguments to used in the
     *                     D-Bus method call
     * @param fd           File descriptor to pass to the D-Bus method
     *
     * @return GVariant*   GVariant object with the results provided by the
     *                     D-Bus method.
     */
    GVariant *SendFD(const TargetPreset::Ptr preset,
                     const std::string &method,
                     GVariant *params,
                     int &fd) const;

    /**
     *  Do a D-Bus call and retrieve aa attached a file descriptor sent
     *  back from the D-Bus method.  The Proxy::Client caller will get
     *  access to whatever this socket is connected to on the service side.
     *
     * @param fd           File descriptor returned by the D-Bus method call
     * @param preset       TargetPreset::Ptr containing the object path
     *                     and interface to perform the method call against
     * @param method       std::string with the D-Bus method to call
     * @param params       GVariant * with the arguments to used in the
     *                     D-Bus method call
     *
     * @return GVariant*   GVariant object with the results provided by the
     *                     D-Bus method.  The file descriptor is available
     *                     via this GVariant object.
     */
    GVariant *GetFD(int &fd,
                    const TargetPreset::Ptr preset,
                    const std::string &method,
                    GVariant *params) const;

    /**
     *  Retrieve the property value of a given property in an object in
     *  within an object interface scope
     *
     * @param object_path    std::string with the D-Bus object path
     * @param interface      std::string with the interface scope in the
     *                       D-Bus object
     * @param property_name  std::string with the D-Bus object property name
     *
     * @return GVariant* object containing the D-Bus property value
     */
    GVariant *GetPropertyGVariant(const std::string &object_path,
                                  const std::string &interface,
                                  const std::string &property_name) const;

    /**
     *  A variant of @Proxy::Client::GetPropertyGVariant() which
     *  uses a TargetPreset::Ptr to provide the object path and interface
     *  of the property being retrieved

     * @param preset         TargetPreset::Ptr containing the object path
     *                       and interface of the property
     * @param property_name  std::string with the D-Bus object property name
     *
     * @return GVariant* object containing the D-Bus property value
     */
    GVariant *GetPropertyGVariant(const TargetPreset::Ptr preset,
                                  const std::string &property_name) const;



    /**
     *  Retrieve the property value of a given property in an object in
     *  within an object interface scope and retrieve the type value into
     *  a C++ variable with the expected data type.
     *
     *  NOTE: The C++ data type MUST match the D-Bus property data type
     *        This method will not do any type casting from the D-Bus
     *        response.
     *
     * @tparam T             C++ data type to retrieve the data as
     * @param object_path    std::string with the D-Bus object path
     * @param interface      std::string with the interface scope in the
     *                       D-Bus object
     * @param property_name  std::string with the D-Bus object property name
     *
     * @return Returns the value as T
     */
    template <typename T>
    T GetProperty(const std::string &object_path,
                  const std::string &interface,
                  const std::string &property_name) const
    {
        GVariant *res = GetPropertyGVariant(object_path, interface, property_name);
        T ret = glib2::Value::Get<T>(res);
        g_variant_unref(res);
        return ret;
    }


    /**
     *  Retrieve the property value of a given property in an object in
     *  within an object interface scope and retrieve the type value into
     *  a C++ variable with the expected data type.
     *
     *  NOTE: The C++ data type MUST match the D-Bus property data type
     *        This method will not do any type casting from the D-Bus
     *        response.
     *
     * @tparam T             C++ data type to retrieve the data as
     * @param preset         TargetPreset::Ptr containing the object path and
     *                       interface of the property
     * @param property_name  std::string with the D-Bus object property name
     *
     * @return Returns the value as T
     */
    template <typename T>
    T GetProperty(const TargetPreset::Ptr preset,
                  const std::string &property_name) const
    {
        GVariant *res = GetPropertyGVariant(preset, property_name);
        T ret = glib2::Value::Get<T>(res);
        g_variant_unref(res);
        return ret;
    }


    /**
     *  Retrieve property array value of a given property in an object in
     *  within an object interface scope and retrieve the type value into
     *  a C++ variable with the expected data type.
     *
     *  NOTE: The C++ data type MUST match the D-Bus property data type
     *        This method will not do any type casting from the D-Bus
     *        response.
     *
     * @tparam T             C++ vector data type to retrieve the data as
     * @param object_path    std::string with the D-Bus object path
     * @param interface      std::string with the interface scope in the
     *                       D-Bus object
     * @param property_name  std::string with the D-Bus object property name
     *
     * @return Returns the value as std::vector<T>
     */
    template <typename T>
    std::vector<T> GetPropertyArray(const std::string &object_path,
                                    const std::string &interface,
                                    const std::string &property_name) const
    {
        GVariant *res = GetPropertyGVariant(object_path, interface, property_name);
        std::vector<T> ret = glib2::Value::ExtractVector<T>(res, nullptr, false);
        g_variant_unref(res);
        return ret;
    }


    /**
     *  Retrieve property array value of a given property in an object in
     *  within an object interface scope and retrieve the type value into
     *  a C++ variable with the expected data type.
     *
     *  NOTE: The C++ data type MUST match the D-Bus property data type
     *        This method will not do any type casting from the D-Bus
     *        response.
     *
     * @tparam T             C++ vector data type to retrieve the data as
     * @param preset         TargetPreset::Ptr containing the object path and
     *                       interface of the property
     * @param property_name  std::string with the D-Bus object property name
     *
     * @return Returns the value as std::vector<T>
     */
    template <typename T>
    std::vector<T> GetPropertyArray(const TargetPreset::Ptr preset,
                                    const std::string &property_name) const
    {
        return GetPropertyArray<T>(preset->object_path, preset->interface, property_name);
    }


    /**
     *  Assign a new value to a D-Bus object property
     *
     * @param object_path    std::string with the D-Bus object path
     * @param interface      std::string with the interface scope in the
     *                       D-Bus object
     * @param property_name  std::string with the D-Bus object property name
     *                       which is to be modified
     * @param params         GVariant object containing the new value of the
     *                       property.  The data type in this container MUST
     *                       match the data type of the D-bus property data
     *                       type.
     */
    void SetPropertyGVariant(const std::string &object_path,
                             const std::string &interface,
                             const std::string &property_name,
                             GVariant *params) const;


    /**
     *  Assign a new value to a D-Bus object property
     *
     * @param preset         TargetPreset::Ptr containing the object path and
     *                       interface of the property
     * @param property_name  std::string with the D-Bus object property name
     *                       which is to be modified
     * @param params         GVariant object containing the new value of the
     *                       property.  The data type in this container MUST
     *                       match the data type of the D-bus property data
     *                       type.
     */
    void SetPropertyGVariant(const TargetPreset::Ptr preset,
                             const std::string &property_name,
                             GVariant *params) const;


    /**
     *  Assign a new value to a D-Bus object property, using
     *  the value data type to the corresponding D-Bus data type
     *
     *  NOTE: The C++ data type MUST match the D-Bus property data type
     *        This method will not do any type casting from the C++
     *        variable type to the D-Bus property type.
     *
     * @tparam T             C++ vector data type of the value variable
     * @param object_path    std::string with the D-Bus object path
     * @param interface      std::string with the interface scope in the
     *                       D-Bus object
     * @param property_name  std::string with the D-Bus object property name
     *                       which is to be modified
     * @param params         GVariant object containing the new value of the
     *                       property.  The data type of T must match the
     *                       D-Bus data type of property value.
     */
    template <typename T>
    void SetProperty(const std::string &object_path,
                     const std::string &interface,
                     const std::string &property_name,
                     T &value) const
    {
        SetPropertyGVariant(object_path,
                            interface,
                            property_name,
                            glib2::Value::Create<T>(value));
    }


    /**
     *  Assign a new value to a D-Bus object property, using
     *  the C++ value data type to the corresponding D-Bus data type
     *
     *  NOTE: The C++ data type MUST match the D-Bus property data type
     *        This method will not do any type casting from the C++
     *        variable type to the D-Bus property type.
     *
     * @tparam T             C++ vector data type of the value variable
     * @param preset         TargetPreset::Ptr containing the object path and
     *                       interface of the property
     * @param property_name  std::string with the D-Bus object property name
     *                       which is to be modified
     * @param params         GVariant object containing the new value of the
     *                       property.  The data type of T must match the
     *                       D-Bus data type of property value.
     */
    template <typename T>
    void SetProperty(const TargetPreset::Ptr preset,
                     const std::string &property_name,
                     T &value) const
    {
        SetPropertyGVariant(preset,
                            property_name,
                            glib2::Value::Create<T>(value));
    }



    friend std::ostream &
    operator<<(std::ostream &os, const Client *prx)
    {
        return os << std::string("Proxy(") << prx->connection << ", "
                  << "'" << prx->destination << "')";
    }


  private:
    Connection::Ptr connection = nullptr; ///< D-Bus connection object
    const std::string destination;        ///< D-Bus service bus name to use

    Client(Connection::Ptr conn, const std::string &dest);
};

} // namespace Proxy
} // namespace DBus
