//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
//  Copyright (C)  Charlie Vigue <charlie.vigue@openvpn.net>
//  Copyright (C)  David Sommerseth <davids@openvpn.net>
//

/**
 * @file object/callbacklink.hpp
 *
 * @brief Declaration of DBus::Object::CallbackLink.  This class contains
 *        the context which is needed by the glib2 callback handlers to
 *        access the right Object::Base object and perform operations
 *        in that D-Bus object.  In addition it contains the needed links
 *        to the DBus::Object::Manager to remove D-Bus objects and the
 *        AsyncProcess::Pool to queue callback events to be handled
 *        asynchronously.
 */

#pragma once

#include <memory>

#include "../async-process.hpp"
#include "base.hpp"
#include "manager.hpp"
#include "path.hpp"


namespace DBus {
namespace Object {

/**
 *  The CallbackLink provides direct access to a DBus::Object::Base
 *  based object when a glib2 gdbus callback function is triggered.  This
 *  is triggered when a D-Bus proxy (client) accesses a D-Bus object on
 *  a service provided over the D-Bus.
 */
class CallbackLink
{
  public:
    using Ptr = std::shared_ptr<CallbackLink>;

    class Exception : public DBus::Exception
    {
      public:
        Exception(const std::string &errmsg);

        Exception(const std::string &sender,
                  const Object::Path &path,
                  const std::string &interf,
                  const std::string &errm);

        Exception(AsyncProcess::Request::UPtr &req,
                  const std::string &errmsg);
    };


    /**
     *  Create a new CallbackLink object
     *
     *  This object keeps the needed links between the DBus::Object::Base,
     *  The DBus::Object::Manager this object belongs to and the
     *  AsyncProcess::Pool used to handle D-Bus method calls asynchronously
     *
     * @param dbus_object   DBus::Object::Base::Ptr (shared_ptr) to the D-Bus object
     * @param om            DBus::Object::Manager::WPtr (weak_ptr) to the Object Manager
     * @param async_pool    DBus::AsyncProcess::Pool::Ptr (shared_ptr) to the process pool
     *
     * @return CallbackLink::Ptr (shared_ptr) to the new CallbackLink object
     */
    [[nodiscard]] static CallbackLink::Ptr Create(Object::Base::Ptr dbus_object,
                                                  Manager::WPtr object_manager,
                                                  AsyncProcess::Pool::Ptr async_pool)
    {
        CallbackLink::Ptr ret = CallbackLink::Ptr(new CallbackLink(dbus_object, object_manager, async_pool));
        return ret;
    }

    virtual ~CallbackLink() noexcept = default;

    /**
     *  Used by the glib2 D-Bus callback functions to create a new
     *  operation request on this object.
     *
     * @param conn       GDBusConnection pointer where the request came from
     * @param sender     std::string containing the unique bus name of the sender
     * @param obj_path   DBus::Object::Path with the D-Bus object path to operate on
     * @param intf_name  std::String with the D-Bus interface to operate on
     *
     * @return AsyncProcess::Request::UPtr to the request object which is to be
     *         passed to QueueBusRequest() to be queued up for processing.
     *         Before the QueueBusRequest() call, the AsyncProcess::Request
     *         object must be further instructed what kind of operation to be
     *         performed.
     */
    AsyncProcess::Request::UPtr NewObjectOperation(GDBusConnection *conn,
                                                   const std::string &sender,
                                                   const Object::Path &obj_path,
                                                   const std::string &intf_name);


    /**
     *  Queue a new AsyncProcess::Request object to be processed via the
     *  thread pool (AsyncProcess::Pool)
     *
     * @param req  AsyncProcess::Request object with the operation to perform
     */
    void QueueOperation(AsyncProcess::Request::UPtr &req);


    ///< DBus::Object this CallbackLink is related to
    Object::Base::Ptr object;

    ///< DBus::Object::Manager this CallbackLink is tied to
    Object::Manager::WPtr manager;


  private:
    /**
     *  The request_pool is not intended to be used outside of the
     *  CallbackLink object; the CallbackLink::QueueOperation() method
     *  is the only method which should have this access
     */
    AsyncProcess::Pool::Ptr request_pool;

    /**
     *  The real CallbackLink() constructor.  This is intended only to be used via
     *  the static CallbackLink::Create() function this class provides.
     */
    CallbackLink(Object::Base::Ptr dbus_object,
                 Manager::WPtr object_manager,
                 AsyncProcess::Pool::Ptr async_pool);
};

} // namespace Object
} // namespace DBus
