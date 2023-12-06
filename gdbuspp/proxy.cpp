//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//


/**
 * @file proxy.cpp
 *
 * @brief Implementation of a base DBus proxy class, used to interact
 *        with a D-Bus service from C++
 */

#include <functional>
#include <string>
#include <sstream>
#include <glib.h>
#include <gio/gio.h>

#include "connection.hpp"
#include "proxy.hpp"
#include "features/debug-log.hpp"
#include "glib2/utils.hpp"

#define DBUS_PROXY_CALL_TIMEOUT 5000

namespace glib2 {

/**
 *  Internal helper class wrapping up the glib2 GDBusProxy related API
 *  into a C++ interface.
 *
 *  The purpose of this is to have easy access to the functionality
 *  and keep all the glib2 memory management related to the GDBusProxy
 *  calls contained within this object.
 *
 *  NOTE: The expected use case is that one Proxy object only does one
 *        single D-Bus call before being torn down.  Doing more calls
 *        with the same may cause undefined behaviour due to state values
 *        left behind from prior calls.
 */
class Proxy
{
  public:
    /**
     *  Prepares the glib2 GDBProxy object and handles all the error management
     *
     * @param connection    DBus::Connection with the main D-Bus connection
     * @param destination_  std::string with the D-Bus service to connect to
     * @param path_         std::string with the target D-Bus object path
     * @param interface_    std::string with the interface within the object
     *                      we want to interact with
     */
    Proxy(DBus::Connection::Ptr connection,
          const std::string &destination_,
          const std::string &path_,
          const std::string &interface_)
        : destination(destination_), object_path(path_), interface(interface_)
    {
        if (!connection->Check())
        {
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         "DBus::Connection is not valid");
        }
        GError *err = nullptr;
        proxy = g_dbus_proxy_new_sync(connection->ConnPtr(),
                                      G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                      nullptr, // GDBusInterfaceInfo
                                      destination.c_str(),
                                      object_path.c_str(),
                                      interface.c_str(),
                                      nullptr, // GCancellable
                                      &err);
        if (!proxy || err)
        {
            if (err)
            {
                throw DBus::Proxy::Exception(destination,
                                             object_path,
                                             interface,
                                             "Failed preparing proxy: "
                                                 + std::string(err->message));
            }
            else
            {
                throw DBus::Proxy::Exception(destination,
                                             object_path,
                                             interface,
                                             "Failed preparing proxy");
            }
        }
    }


    /**
     *  Destructing the Proxy requires to clean up the glib2 GDBusProxy object
     */
    ~Proxy() noexcept
    {
        if (proxy)
        {
            g_object_unref(proxy);
        }
    }


    /**
     *  Wrapping the glib2 g_dbus_proxy_call_sync() call, reusing information
     *  already collected
     *
     * @param method       std::string of the D-Bus method to call
     * @param params       GVariant * containing all the D-Bus method arguments
     * @param no_response  If true, no response is expected by the caller - and
     *                     this method will always return nullptr.  This will
     *                     also disable any error checking possibilities.
     *
     * @return GVariant* is returned containing the arguments from the D-Bus
     *         method
     */
    GVariant *Call(const std::string &method, GVariant *params, bool no_response)
    {
        if (!no_response)
        {
            return do_call(method, params);
        }
        else
        {
            do_call_no_response(method, params);
            return nullptr;
        }
    }


    /**
     *  Do a D-Bus method call and retrieve a file descriptor sent in the
     *  reply.
     *
     *  File descriptors are passed in a "side-channel" and is not part of
     *  of the return signature or response itself (via GVariant*).
     *
     * @param ret_fd       The file descriptor sent back from the service
     * @param method       std::string of the D-Bus method to call
     * @param params       GVariant * containing all the method arguments
     *
     * @return GVariant* is returned containing the arguments from the D-Bus
     *         method as the response
     */
    GVariant *CallGetFD(int *ret_fd, const std::string &method, GVariant *params)
    {
        GVariant *ret = do_call_with_fdlist(method, params, nullptr);

        // The returned file descriptor is extracted in the do_call_with_fdlist()
        // method and preserved in this object as return_fd for
        // code simplicity.  This object is designed to be short-lived
        // (only one object per D-Bus call), so this is fine
        *ret_fd = return_fd;
        return ret;
    }


    /**
     *  Do a D-Bus method call and send a file descriptor to the
     *  D-Bus service receving this call
     *
     *  File descriptors are passed in a "side-channel" is not part of
     *  of the arguments (aka parameters) sent via the GVariant*
     *
     * @param method    std::string of the D-Bus method to call
     * @param params    GVariant * containing all the method arguments
     * @param fd        The file descriptor to send to the service
     *
     * @return GVariant* is returned containing the arguments from the D-Bus
     *         method as the response
     */
    GVariant *CallSendFD(const std::string &method,
                         GVariant *params,
                         int &fd)
    {
        // File descriptor passed on by the caller
        GUnixFDList *caller_fdlist = g_unix_fd_list_new();
        if (!caller_fdlist)
        {
            std::ostringstream erm;
            erm << "Failed allocating file descriptor list for "
                << "'" << method << "'";
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         erm.str());
        }

        GError *error = nullptr;
        if (g_unix_fd_list_append(caller_fdlist, fd, &error) < 0)
        {
            std::ostringstream erm;
            erm << "Failed preparing file descriptor for "
                << "'" << method << "'";
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         erm.str(),
                                         error);
        }
        return do_call_with_fdlist(method, params, caller_fdlist);
    }


  private:
    GDBusProxy *proxy = nullptr;
    const std::string destination;
    const std::string object_path;
    const std::string interface;
    int return_fd = -1;


    /**
     *  Do an ordinary synchronous D-Bus method call in a D-Bus service
     *  This will wait until the serivce has responded or until the
     *  DBUS_PROXY_CALL_TIMEOUT (in ms) timeout hits.
     *
     * @param method       std::string of the method to call
     * @param params       GVariant * containing the arugments to the method
     *
     * @return GVariant* containing the result of the D-Bus method call
     */
    GVariant *do_call(const std::string &method, GVariant *params)
    {
        GError *err = nullptr;
        GVariant *ret = g_dbus_proxy_call_sync(proxy,
                                               method.c_str(),
                                               params,
                                               G_DBUS_CALL_FLAGS_NONE,
                                               DBUS_PROXY_CALL_TIMEOUT,
                                               nullptr,
                                               &err);
        validate_call_response(ret, err, method);
        return ret;
    }


    /**
     *  Do a simple non-blocking D-Bus method call where the result of the
     *  D-Bus method is ignored and thrown away.  This will return instantly
     *  without waiting for the D-Bus method to complete.
     *
     *  The downside of this approach is that there will also be no
     *  error checking of the D-Bus method call.  If the method call fails,
     *  no error messages can be retrieved.
     *
     * @param method       std::string of the method to call
     * @param params       GVariant * containing the arugments to the method
     */
    void do_call_no_response(const std::string &method, GVariant *params)
    {
        GDBUSPP_LOG("Proxy::Client::Call("
                    << "'" << destination << "', "
                    << "'" << object_path << "', "
                    << "'" << interface << "', "
                    << "'" << method << "', "
                    << "params=" << (params ? g_variant_print(params, true) : "(none)")
                    << ") [NO RESPONSE CALL]");

        g_dbus_proxy_call(proxy,
                          method.c_str(),
                          params,
                          G_DBUS_CALL_FLAGS_NONE,
                          DBUS_PROXY_CALL_TIMEOUT,
                          nullptr, // GCancellable
                          nullptr, // glib2 response callback function; not needed
                          nullptr  // user_data, not needed - no callback used
        );
    }


    /**
     *  Similar to the @do_call method, but it also handles sending and
     *  receiving file descriptors to/from the D-Bus method being called
     *
     *  If the method call returns a file descriptor, this is extracted and
     *  stored in the private return_fd variable.  The glib2 implementation
     *  does not put the file descriptors in the GVariant * responses, but
     *  is sent via a "side-channel".
     *
     *  The D-Bus protocol supports passing multiple file descriptors in
     *  both directions.  This implementation will only consider a single
     *  file descriptor in each direction, and only one direction per call.
     *
     * @param method         std::string of the method to call
     * @param params         GVariant * containing the arugments to the method
     * @param caller_fdlist  GUnixFDList * containing the file descriptor to
     *                       send to the D-Bus method.  Can be nullptr if no
     *                       file descriptors is being sent.
     *
     * @return GVariant* containing the result of the D-Bus method call
     */
    GVariant *do_call_with_fdlist(const std::string &method,
                                  GVariant *params,
                                  GUnixFDList *caller_fdlist)
    {
        GDBusConnection *conn = g_dbus_proxy_get_connection(proxy);
        if (!(g_dbus_connection_get_capabilities(conn) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING))
        {
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         "D-Bus connection does not support file descriptor passing");
        }

        GUnixFDList *ret_fd = nullptr;
        GError *error = nullptr;
        GVariant *ret = g_dbus_proxy_call_with_unix_fd_list_sync(proxy,
                                                                 method.c_str(),
                                                                 params, // parameters to method
                                                                 G_DBUS_CALL_FLAGS_NONE,
                                                                 DBUS_PROXY_CALL_TIMEOUT,
                                                                 caller_fdlist, // fd_list (to send)
                                                                 &ret_fd,       // fd from the service
                                                                 nullptr,       // GCancellable
                                                                 &error);
        validate_call_response(ret, error, method);
        if (ret_fd)
        {
            return_fd = g_unix_fd_list_get(ret_fd, 0, &error);
            if (return_fd < 0 || error)
            {
                std::ostringstream erm;
                erm << "Error retrieving file descriptor from "
                    << "'" + method + "'";
                throw DBus::Proxy::Exception(destination,
                                             object_path,
                                             interface,
                                             erm.str(),
                                             error);
            }
            glib2::Utils::unref_fdlist(ret_fd);
        }
        return ret;
    }


    /**
     *  Simple generic helper method to check the GVariant * result from
     *  a D-Bus method call which will throw an error it is considered a
     *  failure.
     *
     * @param res      GVariant * of the result to validate
     * @param error    GError * containing additional error messages;
     *                 may be nullptr if no additional information is available.
     * @param method   std::string with the method being called.  This is just
     *                 used when an exception is thrown, to provide more details
     *                 of what failed.
     */
    void validate_call_response(GVariant *res,
                                GError *error,
                                const std::string &method)
    {
        if (!res || error)
        {
            GDBUSPP_LOG("Proxy::Client::Call("
                        << "'" << destination << "', "
                        << "'" << object_path << "', "
                        << "'" << interface << "', "
                        << "'" << method << "'"
                        << ") ERROR:"
                        << (error ? error->message : "(n/a)"));
            if (res)
            {
                g_variant_unref(res);
            }
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         "Failed calling D-Bus method '" + method + "'",
                                         error);
        }
        GDBUSPP_LOG("Proxy::Client::Call("
                    << "'" << destination << "', "
                    << "'" << object_path << "', "
                    << "'" << interface << "', "
                    << "'" << method << "'"
                    << ") "
                    << "Result: "
                    << (res ? g_variant_print(res, true) : "(none)"));
    }
};

} // namespace glib2


namespace DBus {
namespace Proxy {


Exception::Exception(const std::string &errm, GError *gliberr)
    : DBus::Exception("DBus::Proxy", errm, gliberr)
{
}


Exception::Exception(const std::string &destination,
                     const std::string &path,
                     const std::string &interface,
                     const std::string &errm,
                     GError *gliberr)
    : DBus::Exception(compose_error(destination, path, interface), errm, gliberr)

{
}

const std::string Exception::compose_error(const std::string &destination,
                                           const std::string &path,
                                           const std::string &interface) const
{
    std::ostringstream err;
    err << "Proxy::Client("
        << "'" << destination << "', "
        << "'" << path << "', "
        << "'" << interface << "')";
    return err.str();
}


TargetPreset::TargetPreset(const std::string &object_path_,
                           const std::string &interface_)
    : object_path(object_path_), interface(interface_)
{
}



Client::Client(Connection::Ptr conn, const std::string &dest)
    : connection(conn), destination(dest)
{
}


const std::string &Client::GetDestination() const noexcept
{
    return destination;
}


GVariant *Client::Call(const std::string &object_path,
                       const std::string &interface,
                       const std::string &method,
                       GVariant *params,
                       const bool no_response) const
{
    glib2::Proxy prx(connection,
                     destination,
                     object_path,
                     interface);
    return prx.Call(method, params, no_response);
}


GVariant *Client::Call(const TargetPreset::Ptr preset,
                       const std::string &method,
                       GVariant *params,
                       const bool no_response) const
{
    glib2::Proxy prx(connection,
                     destination,
                     preset->object_path,
                     preset->interface);
    return prx.Call(method, params, no_response);
}


GVariant *Client::GetFD(int &fd,
                        const TargetPreset::Ptr preset,
                        const std::string &method,
                        GVariant *params) const
{
    glib2::Proxy prx(connection,
                     destination,
                     preset->object_path,
                     preset->interface);
    return prx.CallGetFD(&fd, method, params);
}


GVariant *Client::SendFD(const TargetPreset::Ptr preset,
                         const std::string &method,
                         GVariant *params,
                         int &fd) const
{
    glib2::Proxy prx(connection,
                     destination,
                     preset->object_path,
                     preset->interface);
    return prx.CallSendFD(method, params, fd);
}


GVariant *Client::GetPropertyGVariant(const std::string &object_path,
                                      const std::string &interface,
                                      const std::string &property_name) const
{
    glib2::Proxy prx(connection,
                     destination,
                     object_path,
                     "org.freedesktop.DBus.Properties");

    // The Get method needs the property interface scope of the property
    // and the property name
    GVariant *params = g_variant_new("(ss)",
                                     interface.c_str(),
                                     property_name.c_str());

    GVariant *resp = prx.Call("Get", params, false);

    // Property results are wrapped into a "variant" type, which
    // needs to be unpacked to retrieve the property value.
    GVariant *child = g_variant_get_child_value(resp, 0);
    GVariant *ret = g_variant_get_variant(child);
    g_variant_unref(child);
    g_variant_unref(resp);

    return ret;
}


GVariant *Client::GetPropertyGVariant(const TargetPreset::Ptr preset,
                                      const std::string &property_name) const
{
    return GetPropertyGVariant(preset->object_path, preset->interface, property_name);
}


void Client::SetPropertyGVariant(const std::string &object_path,
                                 const std::string &interface,
                                 const std::string &property_name,
                                 GVariant *value) const
{
    glib2::Proxy prx(connection,
                     destination,
                     object_path,
                     "org.freedesktop.DBus.Properties");

    // The Set method needs the property interface scope of the property
    // and the property name
    GVariant *params = g_variant_new("(ssv)",
                                     interface.c_str(),
                                     property_name.c_str(),
                                     value);

    prx.Call("Set", params, true);
}


void Client::SetPropertyGVariant(const TargetPreset::Ptr preset,
                                 const std::string &property_name,
                                 GVariant *params) const
{
    SetPropertyGVariant(preset->object_path, preset->interface, property_name, params);
}

} // namespace Proxy
} // namespace DBus
