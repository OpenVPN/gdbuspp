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
#include "object/path.hpp"
#include "proxy/utils.hpp"

#define DBUS_PROXY_CALL_TIMEOUT 5000


namespace glib2 {

namespace proxy_callbacks {

void result_handler(GDBusProxy *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = nullptr;
    GVariant *result = g_dbus_proxy_call_finish(proxy,
                                                res,
                                                &error);
    g_variant_unref(result);
}

} // namespace proxy_callbacks

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
     * @param error_details_ std::string with details to use in the
     *                       exception in case of an error
     */
    Proxy(DBus::Connection::Ptr connection,
          const std::string &destination_,
          const DBus::Object::Path &path_,
          const std::string &interface_,
          const std::string &error_details_ = "")
        : destination(destination_), object_path(path_), interface(interface_)
    {
        if (!connection || !connection->Check())
        {
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         error_details_,
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
                                             error_details_,
                                             "Failed preparing proxy: "
                                                 + std::string(err->message));
            }
            else
            {
                throw DBus::Proxy::Exception(destination,
                                             object_path,
                                             interface,
                                             error_details_,
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
                                         method,
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
                                         method,
                                         erm.str(),
                                         error);
        }
        return do_call_with_fdlist(method, params, caller_fdlist);
    }


  private:
    GDBusProxy *proxy = nullptr;
    const std::string destination;
    const DBus::Object::Path object_path;
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
        GDBUSPP_LOG("Proxy::Client::Call("
                    << "'" << destination << "', "
                    << "'" << object_path << "', "
                    << "'" << interface << "', "
                    << "'" << method << "', "
                    << "params=" << glib2::Utils::DumpToString(params)
                    << ")");
        GError *err = nullptr;
        GVariant *ret = g_dbus_proxy_call_sync(proxy,
                                               method.c_str(),
                                               glib2::Value::TupleWrap(params),
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
                    << "params=" << glib2::Utils::DumpToString(params)
                    << ") [NO RESPONSE CALL]");

        g_dbus_proxy_call(proxy,
                          method.c_str(),
                          glib2::Value::TupleWrap(params),
                          G_DBUS_CALL_FLAGS_NONE,
                          DBUS_PROXY_CALL_TIMEOUT,
                          nullptr, // GCancellable
                          (GAsyncReadyCallback)proxy_callbacks::result_handler,
                          nullptr // user_data, not needed - no callback used
        );
    }


    /**
     *  Similar to the do_call() method, but it also handles sending and
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
                                         method,
                                         "D-Bus connection does not support file descriptor passing");
        }

        GDBUSPP_LOG("Proxy::Client::"
                    << (caller_fdlist ? "SendFD" : "GetFD")
                    << "("
                    << "'" << destination << "', "
                    << "'" << object_path << "', "
                    << "'" << interface << "', "
                    << "'" << method << "', "
                    << "params=" << glib2::Utils::DumpToString(params)
                    << ") ");

        GUnixFDList *ret_fd = nullptr;
        GError *error = nullptr;
        GVariant *ret = g_dbus_proxy_call_with_unix_fd_list_sync(proxy,
                                                                 method.c_str(),
                                                                 glib2::Value::TupleWrap(params), // parameters to method
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
                                             method,
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
            GDBUSPP_LOG("Proxy::Client call result ("
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
            std::string err;
            err = (!error ? "Failed calling D-Bus method '" + method + "'" : "");
            throw DBus::Proxy::Exception(destination,
                                         object_path,
                                         interface,
                                         method,
                                         err,
                                         error);
        }
        GDBUSPP_LOG("Proxy::Client call result ("
                    << "'" << destination << "', "
                    << "'" << object_path << "', "
                    << "'" << interface << "', "
                    << "'" << method << "'"
                    << ") "
                    << "Result: "
                    << glib2::Utils::DumpToString(res));
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
                     const DBus::Object::Path &path,
                     const std::string &interface,
                     const std::string &method,
                     const std::string &errm,
                     GError *gliberr)
    : DBus::Exception(compose_error(destination, path, interface, method),
                      errm,
                      gliberr)

{
}

const std::string Exception::compose_error(const std::string &destination,
                                           const DBus::Object::Path &path,
                                           const std::string &interface,
                                           const std::string &method) const
{
    std::ostringstream err;
    std::ostringstream method_info{};
    if (!method.empty())
    {
        method_info << "', '" << method;
    }
    err << "Proxy::Client("
        << "'" << destination << "', "
        << "'" << path << "', "
        << "'" << interface << method_info.str()
        << "')";
    return err.str();
}


TargetPreset::TargetPreset(const Object::Path &object_path_,
                           const std::string &interface_)
    : object_path(object_path_), interface(interface_)
{
}



Client::Client(Connection::Ptr conn, const std::string &dest, uint8_t timeout)
    : connection(conn), destination(dest)
{
    if ("org.freedesktop.DBus" != dest)
    {
        // Don't do this if querying org.freedesktop.DBus.
        // First, this need to be running anyhow - and it would
        // result in a recursion, since DBusServiceQuery uses this Client
        // implementation
        auto srvqry = Proxy::Utils::DBusServiceQuery::Create(connection);
        if (!srvqry->CheckServiceAvail(destination, timeout))
        {
            throw DBus::Proxy::Exception("Service '"
                                         + destination
                                         + "' cannot be reached");
        }
    }
}


const std::string &Client::GetDestination() const noexcept
{
    return destination;
}


GVariant *Client::Call(const Object::Path &object_path,
                       const std::string &interface,
                       const std::string &method,
                       GVariant *params,
                       const bool no_response) const
{
    glib2::Proxy prx(connection,
                     destination,
                     object_path,
                     interface,
                     method);
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
                     preset->interface,
                     method);
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
                     preset->interface,
                     method);
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
                     preset->interface,
                     method);
    return prx.CallSendFD(method, params, fd);
}


GVariant *Client::GetPropertyGVariant(const Object::Path &object_path,
                                      const std::string &interface,
                                      const std::string &property_name) const
{
    glib2::Proxy prx(connection,
                     destination,
                     object_path,
                     "org.freedesktop.DBus.Properties",
                     "Get(" + property_name + ")");

    // The Get method needs the property interface scope of the property
    // and the property name
    GVariantBuilder *params = glib2::Builder::Create("(ss)");
    glib2::Builder::Add(params, interface);
    glib2::Builder::Add(params, property_name);

    GVariant *resp = prx.Call("Get", glib2::Builder::Finish(params), false);

    // Property results are wrapped into a "variant" type, which
    // needs to be unpacked to retrieve the property value.
    auto *ret = glib2::Value::Extract<GVariant *>(resp, 0);
    g_variant_unref(resp);

    return ret;
}


GVariant *Client::GetPropertyGVariant(const TargetPreset::Ptr preset,
                                      const std::string &property_name) const
{
    return GetPropertyGVariant(preset->object_path, preset->interface, property_name);
}


void Client::SetPropertyGVariant(const Object::Path &object_path,
                                 const std::string &interface,
                                 const std::string &property_name,
                                 GVariant *value) const
{
    glib2::Proxy prx(connection,
                     destination,
                     object_path,
                     "org.freedesktop.DBus.Properties",
                     "Set(" + property_name + ")");

    // The Set method needs the property interface scope of the property
    // and the property name
    GVariantBuilder *params = glib2::Builder::Create("(ssv)");
    glib2::Builder::Add(params, interface);
    glib2::Builder::Add(params, property_name);
    glib2::Builder::Add(params, value, glib2::DataType::DBUS_TYPE_VARIANT);

    // Even though there is no response expected, this need to be
    // a synchronous call.  Programs only setting a property and then
    // exiting will experience the property often not be modified at all.
    // Doing this synchronously ensures we wait until the request has been
    // processed by the service.
    GVariant *r = prx.Call("Set", glib2::Builder::Finish(params), false);
    g_variant_unref(r);
}


void Client::SetPropertyGVariant(const TargetPreset::Ptr preset,
                                 const std::string &property_name,
                                 GVariant *params) const
{
    SetPropertyGVariant(preset->object_path, preset->interface, property_name, params);
}

} // namespace Proxy
} // namespace DBus
