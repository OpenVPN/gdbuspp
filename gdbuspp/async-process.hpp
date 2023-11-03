//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

#pragma once

/**
 * @file async-process.hpp
 *
 * @brief C++ wrapper for the glib2 g_thread_pool interface
 */

#include <memory>
#include <string>
#include <sstream>
#include <gio/gio.h>

#include "exceptions.hpp"
#include "object/operation.hpp"

namespace DBus {

// Forward declaration; needed by AsyncProcess below
namespace Object {
class Base;
}; // namespace Object



/**
 *  The AsyncProcess scope contains a wrapper for glib2's thread pool API
 *  with adoptations to handle D-Bus requests, specific for D-Bus objects
 *
 *  glib2 documentation: https://docs.gtk.org/glib/struct.ThreadPool.html
 *
 *  The basic conecpt is around using a pool of processing threads, which
 *  receives a pointer to some data to work on.  The thread pool is by
 *  designed to be half of the amount of available CPU cores.
 *
 */
namespace AsyncProcess {

class Exception : public DBus::Exception
{
  public:
    Exception(const std::string &err);
};


/**
 *  The AsyncProcess::Request contains information related to a specific
 *  incoming D-Bus request.  It carries information as the D-Bus connection,
 *  the D-Bus object to operate on and what kind of operation and values
 *  needed to perform the operation.
 *
 *  The concept here is that this request will end up in the right
 *  DBus::Object callback function which will do the right operation.
 *
 *  It is the glib2 thread pool feature which will queue up and dispatch
 *  these requests when there are resources available.
 *
 */
struct Request
{
  public:
    using Ptr = std::shared_ptr<Request>;
    using UPtr = std::unique_ptr<Request>;

    /// glib2 D-Bus connection object where the request came from
    const GDBusConnection *dbusconn;

    /// aka: Object::Base::Ptr; the DBus::Object::Base object being processed
    std::shared_ptr<Object::Base> object;

    /// The D-Bus caller's unique bus name
    const std::string sender;

    /// The object operation this request wants to perform
    Object::Operation request_type = Object::Operation::NONE;

    /// Used with Object::Operation::METHOD_CALL, the method name being called
    std::string method = {};

    /// Used with Object::Operation::PROPERTY_GET and
    /// Object::Operation::PROPERTY_GET, the property being accessed
    std::string property = {};

    /// The arguments passed on from the D-Bus caller
    GVariant *params = nullptr;

    /// Used with ReqType::METHOD, where the result of the callback function is returned
    GDBusMethodInvocation *invocation = nullptr;

    /// Default error domain in case of reporting errors back
    std::string error_domain = "net.openvpn.gdbuspp.request";


    /**
     *  Creates a new AsyncProcess::Request object for a specific D-Bus object.
     *  These objects are created via the glib2::Callback C functions which
     *  will end up in the DBus::Object::Base callback methods to be processed
     *  in that object.
     *
     * @param gdbus_conn      glib2 GDBusConnection object where the request came from
     * @param dbus_object     DBus::Object::Ptr (shared_ptr) to the C++ object side
     * @param sender          std::string containing the unique bus name of the sender
     * @param object_path     std::string with the D-Bus object path to operate on
     * @param interface       std::String with the D-Bus interface to operate on
     *
     * @returns a Request::Ptr (unique_ptr) to the new async request
     */
    static Request::UPtr Create(GDBusConnection *gdbus_conn,
                                std::shared_ptr<Object::Base> dbus_object,
                                const std::string &sender,
                                const std::string &object_path,
                                const std::string &interface)
    {
        return UPtr(new Request(gdbus_conn,
                                dbus_object,
                                sender,
                                object_path,
                                interface));
    }

    virtual ~Request() noexcept = default;


    /**
     *  Prepare this request as a D-Bus method call inside the given D-Bus
     *  object
     *
     * @param meth   std::string with the method name being called
     * @param prms   GVariant pointer to the parameters to the method call
     * @param invoc  GDBusMethodInvocation pointer needed to respond to the
     *               D-Bus method call
     */
    void MethodCall(const std::string &meth,
                    GVariant *prms,
                    GDBusMethodInvocation *invoc) noexcept;



    /**
     *  Prepare this request to retrieve a D-Bus object property value
     *
     * @param propname  std::string containing the property name to extract the
     *                  value for
     */
    void GetProperty(const std::string &propname) noexcept;


    /**
     *  Prepare this request to set a new value to a property in the given
     *  D-Bus object
     *
     * @param propname   std::string containing the property name where to
     *                   set the property value
     * @param prms       GVariant* containing the new value to use for the
     *                   property
     */
    void SetProperty(const std::string &propname, GVariant *prms) noexcept;


    /**
     *  Standard iostream compliant stream operator, providing a human readable
     *  representation of the important information carried by this Request object.
     *
     *  This is mostly useful in error reporting and debugging contexts.
     */
    friend std::ostream &operator<<(std::ostream &os, const Request::UPtr &req)
    {
        std::ostringstream r;
        r << "{AsyncPool::Request " << req->object
          << ": Type=" << Object::OperationString(req->request_type);

        switch (req->request_type)
        {
        case Object::Operation::METHOD_CALL:
            r << ", method=" << req->method;
            break;
        case Object::Operation::PROPERTY_SET:
        case Object::Operation::PROPERTY_GET:
            r << ", property=" << req->property;
        case Object::Operation::NONE:
            break;
        }
        r << "}";

        return os << r.str();
    }


  private:
    /**
     *  This is the base information required for all kind
     *  of DBus object operations.
     *
     *  This must be called via the static @Create() method
     *
     * @param gdbus_conn      GDBusConnection pointer where the request came from
     * @param dbus_object     DBus::Object::Ptr (shared_ptr) to the C++ object side
     * @param sender          std::string containing the unique bus name of the sender
     * @param object_path     std::string with the D-Bus object path to operate on
     * @param interface       std::String with the D-Bus interface to operate on
     */
    Request(GDBusConnection *gdbus_conn,
            std::shared_ptr<Object::Base> dbus_object,
            const std::string &sender,
            const std::string &object_path,
            const std::string &interface);
};



/**
 *  The AsyncProcess:Pool prepares and configures the glib2 thread pool
 *  feature.  This is also the object receiving AsyncProcess::Request objects
 *  to be queued for processing
 */
class Pool
{
  public:
    using Ptr = std::shared_ptr<Pool>;

    /**
     * Construct a new Pool object
     *
     * The glib2 thread pool is allocated and prepared when the
     * constructed returns
     */
    [[nodiscard]] static Pool::Ptr Create()
    {
        return Pool::Ptr(new Pool());
    }


    /**
     * Destroy the Pool object
     *
     * This will also release the glib2 thread pool once all pushed requests
     * has been processed.  This will block the destructor to return until
     * the processing queue is empty.  New requests are not allowed to be added
     * in the mean time.
     */
    ~Pool() noexcept;

    /**
     *  Pushes a new AsyncProcess::Request to queued up for processing
     *
     * @param req  AsyncProcess::Request::UPtr  to the data to be processed
     */
    void PushCallback(Request::UPtr &req);


  private:
    GThreadPool *pool = nullptr;

    Pool();
};

}; // namespace AsyncProcess

}; // namespace DBus
